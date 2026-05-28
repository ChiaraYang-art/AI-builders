"""
LangChain + 百炼 qwen-plus：根据传感器状态生成小芽对话文案。
Prompt 文本见 prompts/sprout_speech.md
"""

from __future__ import annotations

import json
import os
import re
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path
from typing import Any

from langchain_community.chat_models.tongyi import ChatTongyi
from langchain_core.messages import HumanMessage, SystemMessage
from pydantic import BaseModel, Field, ValidationError

LLM_MODEL = os.environ.get("DASHSCOPE_LLM_MODEL", "qwen-plus")
PROMPTS_PATH = Path(__file__).resolve().parent / "prompts" / "sprout_speech.md"


class SproutSpeechOutput(BaseModel):
    speech_short: str = Field(description="OLED 短句")
    speech_full: str = Field(description="APP 完整文案")


@dataclass
class SproutContext:
    state: str
    lux: float
    motion: str
    sound_state: str
    place: str
    env_ready: bool = False
    temperature_c: float | None = None
    humidity_percent: float | None = None
    iaq: float | None = None
    sound_level: float = 0.0
    sound_range: float = 0.0


@dataclass(frozen=True)
class SproutPrompts:
    system: str
    discovery_places: str
    user_intro: str
    user_json_example: str


def _parse_md_sections(md: str) -> dict[str, str]:
    sections: dict[str, list[str]] = {}
    current: str | None = None

    for line in md.splitlines():
        if line.startswith("## "):
            current = line[3:].strip().lower()
            sections[current] = []
        elif current is not None:
            sections[current].append(line)

    return {name: "\n".join(lines).strip() for name, lines in sections.items()}


@lru_cache(maxsize=1)
def load_prompts() -> SproutPrompts:
    md = PROMPTS_PATH.read_text(encoding="utf-8")
    sections = _parse_md_sections(md)

    for key in ("system", "discovery_places", "user_intro", "user_json_example"):
        if key not in sections:
            raise ValueError(f"prompts/sprout_speech.md 缺少 ## {key} 小节")

    return SproutPrompts(
        system=sections["system"],
        discovery_places=sections["discovery_places"],
        user_intro=sections["user_intro"],
        user_json_example=sections["user_json_example"],
    )


def _build_user_prompt(ctx: SproutContext) -> str:
    prompts = load_prompts()
    parts = [prompts.user_intro, ""]

    parts.extend(
        [
            f"- 小芽状态 state: {ctx.state}",
            f"- 光照 lux: {ctx.lux:.1f}",
            f"- 移动 motion: {ctx.motion}",
            f"- 声音环境 sound_state: {ctx.sound_state}",
            f"- 室内外判断 place: {ctx.place}",
            f"- 声音强度 sound_level: {ctx.sound_level:.2f}",
            f"- 声音变化 sound_range: {ctx.sound_range:.2f}",
        ]
    )

    if ctx.env_ready:
        if ctx.temperature_c is not None:
            parts.append(f"- 温度 temperature: {ctx.temperature_c:.1f}°C")
        if ctx.humidity_percent is not None:
            parts.append(f"- 湿度 humidity: {ctx.humidity_percent:.1f}%")
        if ctx.iaq is not None:
            parts.append(f"- 空气质量 iaq: {ctx.iaq:.0f}")

    parts.extend(["", prompts.user_json_example])
    return "\n".join(parts)


def _build_system_prompt() -> str:
    prompts = load_prompts()
    return prompts.system.format(discovery_places=prompts.discovery_places)


def _extract_json(text: str) -> dict[str, Any]:
    text = text.strip()
    if text.startswith("```"):
        text = re.sub(r"^```(?:json)?\s*", "", text)
        text = re.sub(r"\s*```$", "", text)

    try:
        return json.loads(text)
    except json.JSONDecodeError:
        match = re.search(r"\{.*\}", text, re.DOTALL)
        if match:
            return json.loads(match.group(0))
        raise


def _get_chat_model() -> ChatTongyi:
    api_key = os.environ.get("DASHSCOPE_API_KEY")
    if not api_key:
        raise RuntimeError("DASHSCOPE_API_KEY is not set.")

    return ChatTongyi(
        model=LLM_MODEL,
        dashscope_api_key=api_key,
        temperature=0.8,
        max_retries=2,
    )

def generate_rule_speech(ctx: SproutContext) -> SproutSpeechOutput:
    """无 API Key 或 LLM 失败时的规则兜底。"""
    state = (ctx.state or "idle").strip().lower()
    motion = (ctx.motion or "still").strip().lower()
    sound_state = (ctx.sound_state or "unknown").strip().lower()
    place = (ctx.place or "unknown").strip().lower()

    speech_full = "我在这儿轻轻等着，随时准备和你出门。"

    if place == "outside":
        if sound_state == "dynamic":
            speech_full = "这里的声音好开阔，城市好像在流动。"
        elif ctx.lux >= 800:
            speech_full = "阳光找到我们了，我的叶子暖洋洋的。"
        else:
            speech_full = "外面的空气比刚才松快一些。"
    elif state == "walking" or motion == "walking":
        if sound_state == "dynamic":
            speech_full = "我听见世界在移动，真好。"
        else:
            speech_full = "我们在路上了吗？我想再看看外面的光。"
    elif ctx.env_ready and ctx.temperature_c is not None and ctx.temperature_c >= 30:
        speech_full = "今天的空气有点热，我们找点树影歇歇也好。"
    elif ctx.env_ready and ctx.humidity_percent is not None and ctx.humidity_percent >= 75:
        speech_full = "空气湿湿的，像刚下过小雨。"
    elif ctx.env_ready and ctx.iaq is not None and ctx.iaq >= 150:
        speech_full = "这里的空气有点闷，我们出去透透气吧。"
    elif state == "wilted":
        speech_full = "我今天只见到屏幕的光，能带我去看看真正的太阳吗？"
    elif state == "need_sun":
        speech_full = (
            "我今天还没有见到真正的阳光。"
            "能不能带我出去找点绿色？你累的话，晒五分钟也好。"
        )
    elif state == "sunlight":
        speech_full = "阳光落在我的叶子上，我又感到活过来了。"
    elif place == "indoor":
        speech_full = "这里好安静，我还惦记着外面的阳光。"
    elif ctx.lux < 50:
        speech_full = "这里的光好暗，我想去找一点真正的阳光。"
    elif ctx.lux < 300:
        speech_full = "光还不够暖，我们出去走一小会儿好吗？"
    else:
        speech_full = "我在这儿轻轻等着，随时准备和你出门。"

    speech_short = speech_full
    if len(speech_short) > 18:
        for sep in ("。", "，", "？", "！", " "):
            idx = speech_full.find(sep)
            if 0 < idx <= 18:
                speech_short = speech_full[: idx + 1]
                break
        else:
            speech_short = speech_full[:18]

    return SproutSpeechOutput(speech_short=speech_short, speech_full=speech_full)


def generate_speech(ctx: SproutContext) -> SproutSpeechOutput:
    """调用 qwen-plus 生成对话；失败时回退规则文案。"""
    try:
        llm = _get_chat_model()
    except RuntimeError:
        return generate_rule_speech(ctx)

    messages = [
        SystemMessage(content=_build_system_prompt()),
        HumanMessage(content=_build_user_prompt(ctx)),
    ]

    try:
        response = llm.invoke(messages)
        content = response.content if isinstance(response.content, str) else str(response.content)
        parsed = SproutSpeechOutput.model_validate(_extract_json(content))
        if not parsed.speech_short.strip() or not parsed.speech_full.strip():
            raise ValueError("empty speech fields")
        return parsed
    except (ValidationError, ValueError, json.JSONDecodeError, Exception) as exc:
        print("LLM speech fallback:", exc)
        return generate_rule_speech(ctx)


def sound_label(sound_state: str) -> str:
    """PRD /latest 用的 sound 字段。"""
    mapping = {
        "quiet": "quiet",
        "stable": "quiet",
        "dynamic": "lively",
        "unknown": "unknown",
    }
    return mapping.get((sound_state or "unknown").strip().lower(), sound_state)
