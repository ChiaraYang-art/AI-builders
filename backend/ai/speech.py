"""
LangChain + 百炼 qwen-plus：根据传感器状态生成小芽对话文案。
Prompt 文本见 prompts/sprout_speech_gentle.md
"""

from __future__ import annotations

import json
import os
import random
from dataclasses import dataclass
from functools import lru_cache
from typing import Any

from langchain_community.chat_models.tongyi import ChatTongyi
from langchain_core.messages import HumanMessage, SystemMessage
from pydantic import BaseModel, Field, ValidationError

from ai.common import extract_json
from config import LLM_MODEL, PROMPTS_DIR

PROMPTS_PATH = PROMPTS_DIR / "sprout_speech_gentle.md"
CHITCHAT_CHANCE = 0.25

STATE_CHITCHAT_LINES = {
    "idle": [
        "叶子竖起来听你说话。",
        "小小一株，大大好奇。",
        "你好呀，我是口袋小芽。",
        "啦啦啦～",
        "你今天过得好嘛？",
        "工作累了不要忘记出门休息哦！",
        "人，你好，今天也要开心点！",
    ],
    "wilted": [
        "救救我的绿色心情。",
        "我正在进行戏剧性低头。",
        "好困好困！",
    ],
    "need_sun": [
        "我快要鼓起勇气开花了。",
        "晒一下，我会闪闪发亮。",
    ],
    "sunlight": [
        "找到太阳啦，开派对！",
        "我正在光合作用。",
        "你看，我长高了一点点！",
        "我的花花在跳舞。",
        "开花模式正式启动！",
    ],
    "walking": [
        "哇，出门模式启动！",
        "我们是在冒险吗？我同意！",
        "这条路把叶子晃开心了。",
        "我是你的散步小伙伴。",
        "新地方会长出新想法。",
    ],
}

WALK_HINTS = {
    "light": "Light Walk：本轮请自然鼓励用户出门晒晒太阳、找一束光或观察影子。",
    "sound": "Sound Walk：本轮请自然鼓励用户出门听听城市声音、寻找温柔的小声响。",
    "color": "Color Walk：本轮请自然鼓励用户出门寻找颜色、收集街边的色彩。",
    "local": "Local Discovery：本轮请自然使用预设地点信息，邀请用户去附近发现一个小细节。",
}


class SproutSpeechOutput(BaseModel):
    speech_short: str = Field(description="OLED 短句")
    speech_full: str = Field(description="APP 完整文案")
    suggested_walk_type: str | None = None


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
            raise ValueError(f"prompts/sprout_speech_gentle.md 缺少 ## {key} 小节")

    return SproutPrompts(
        system=sections["system"],
        discovery_places=sections["discovery_places"],
        user_intro=sections["user_intro"],
        user_json_example=sections["user_json_example"],
    )


def _choose_outdoor_walk_type(ctx: SproutContext) -> str | None:
    state = (ctx.state or "idle").strip().lower()
    place = (ctx.place or "unknown").strip().lower()

    if state not in {"idle", "wilted", "need_sun"} or place != "indoor":
        return None

    roll = random.random()
    if roll < 0.50:
        return "light"
    if roll < 0.70:
        return "sound"
    if roll < 0.90:
        return "color"
    return "local"


def _shorten_speech(speech_full: str) -> str:
    if len(speech_full) <= 18:
        return speech_full

    for sep in ("。", "，", "？", "！", " "):
        idx = speech_full.find(sep)
        if 0 < idx <= 18:
            return speech_full[: idx + 1]

    return speech_full[:18]


def _generate_chitchat_speech(ctx: SproutContext) -> SproutSpeechOutput:
    state = (ctx.state or "idle").strip().lower()
    lines = STATE_CHITCHAT_LINES.get(state) or STATE_CHITCHAT_LINES["idle"]
    speech_full = random.choice(lines)
    return SproutSpeechOutput(
        speech_short=_shorten_speech(speech_full),
        speech_full=speech_full,
        suggested_walk_type=None,
    )


def _generate_rule_walk_speech(
    ctx: SproutContext,
    suggested_walk_type: str | None = None,
) -> SproutSpeechOutput | None:
    walk_type = suggested_walk_type or _choose_outdoor_walk_type(ctx)
    if not walk_type:
        return None

    if walk_type == "light":
        speech_full = random.choice(
            [
                "小芽想出门找一束真正的阳光，我们去看看哪段影子最会跳舞吧？",
                "叶子想吃一点亮亮的小点心，带我出去晒五分钟太阳好不好？",
                "今天可以来一场光线散步吗？我想收集一小块金色的地面。",
            ]
        )
    elif walk_type == "sound":
        speech_full = random.choice(
            [
                "我们去听听城市今天的声音吧，说不定路上有很温柔的小节奏。",
                "小芽竖起叶子啦，想出门收集三个让人开心的声音。",
                "带我出去听一听风和脚步声吧，我想知道街道今天在说什么。",
            ]
        )
    elif walk_type == "color":
        speech_full = random.choice(
            [
                "要不要一起出门找五个绿色的东西？小芽想看看外面的颜色菜单。",
                "今天来一场颜色散步吧，我们去找一抹最惊喜的蓝色或粉色。",
                "带我出去看看今天是什么颜色吧，我的叶子已经开始好奇了。",
            ]
        )
    else:
        speech_full = random.choice(
            [
                "要不要去大学路转角的小书店看看？我听说门口有几盆绿植。",
                "附近发现任务来了，我们去国权路那边看看咖啡店的新菜单吧？",
                "可以带我去复旦附近的面包店吗？我想闻闻抹茶味的风。",
            ]
        )

    return SproutSpeechOutput(
        speech_short=_shorten_speech(speech_full),
        speech_full=speech_full,
        suggested_walk_type=walk_type,
    )


def _build_user_prompt(ctx: SproutContext, suggested_walk_type: str | None = None) -> str:
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

    if suggested_walk_type:
        parts.extend(["", f"- 出门建议倾向 walk_hint: {WALK_HINTS[suggested_walk_type]}"])

    parts.extend(["", prompts.user_json_example])
    return "\n".join(parts)


def _build_system_prompt() -> str:
    prompts = load_prompts()
    return prompts.system.format(discovery_places=prompts.discovery_places)


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


def generate_rule_speech(
    ctx: SproutContext,
    suggested_walk_type: str | None = None,
) -> SproutSpeechOutput:
    state = (ctx.state or "idle").strip().lower()
    motion = (ctx.motion or "still").strip().lower()
    sound_state = (ctx.sound_state or "unknown").strip().lower()
    place = (ctx.place or "unknown").strip().lower()

    walk_speech = _generate_rule_walk_speech(ctx, suggested_walk_type)
    if walk_speech:
        return walk_speech

    speech_full = "我在这儿轻轻等着，随时准备和你出门。"

    if place == "outside":
        if sound_state in {"active", "dynamic"}:
            speech_full = "这里的声音好开阔，城市好像在流动。"
        elif ctx.lux >= 800:
            speech_full = "阳光找到我们了，我的叶子暖洋洋的。"
        else:
            speech_full = "外面的空气比刚才松快一些。"
    elif state == "walking" or motion == "walking":
        if sound_state in {"active", "dynamic"}:
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
        speech_full = "我今天还没有见到真正的阳光，能不能带我出去找点绿色？"
    elif state == "sunlight":
        speech_full = "阳光落在我的叶子上，我又感到活过来了。"
    elif place == "indoor":
        speech_full = "这里好安静，我还惦记着外面的阳光。"
    elif ctx.lux < 50:
        speech_full = "这里的光好暗，我想去找一点真正的阳光。"
    elif ctx.lux < 300:
        speech_full = "光还不够暖，我们出去走一小会儿好吗？"

    return SproutSpeechOutput(
        speech_short=_shorten_speech(speech_full),
        speech_full=speech_full,
        suggested_walk_type=None,
    )


def generate_speech(ctx: SproutContext) -> SproutSpeechOutput:
    if random.random() < CHITCHAT_CHANCE:
        return _generate_chitchat_speech(ctx)

    suggested_walk_type = _choose_outdoor_walk_type(ctx)

    try:
        llm = _get_chat_model()
    except RuntimeError:
        return generate_rule_speech(ctx, suggested_walk_type)

    messages = [
        SystemMessage(content=_build_system_prompt()),
        HumanMessage(content=_build_user_prompt(ctx, suggested_walk_type)),
    ]

    try:
        response = llm.invoke(messages)
        content = response.content if isinstance(response.content, str) else str(response.content)
        parsed = SproutSpeechOutput.model_validate(extract_json(content))
        if not parsed.speech_short.strip() or not parsed.speech_full.strip():
            raise ValueError("empty speech fields")
        parsed.suggested_walk_type = suggested_walk_type
        return parsed
    except (ValidationError, ValueError, json.JSONDecodeError, Exception) as exc:
        print("LLM speech fallback:", exc)
        return generate_rule_speech(ctx, suggested_walk_type)


def sound_label(sound_state: str) -> str:
    mapping = {
        "quiet": "quiet",
        "stable": "quiet",
        "active": "active",
        "dynamic": "active",
        "intense": "intense",
        "unknown": "unknown",
    }
    return mapping.get((sound_state or "unknown").strip().lower(), sound_state)
