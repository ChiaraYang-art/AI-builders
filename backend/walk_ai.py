"""散步媒体分析：照片识色、声音标签、日记生成（LLM + 规则兜底）。"""

from __future__ import annotations

import json
import os
import random
import re
from pathlib import Path
from typing import Any

from langchain_community.chat_models.tongyi import ChatTongyi
from langchain_core.messages import HumanMessage, SystemMessage

COLOR_FALLBACKS = [
    "light green",
    "dark green",
    "yellow green",
    "mint green",
    "forest green",
]

SOUND_FALLBACKS = [
    ["birdsong", "leaves", "footsteps"],
    ["traffic", "human voices", "wind"],
    ["birdsong", "distant traffic", "footsteps"],
    ["wind", "footsteps", "city hum"],
]


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


def _llm() -> ChatTongyi | None:
    api_key = os.environ.get("DASHSCOPE_API_KEY")
    if not api_key:
        return None

    model = os.environ.get("DASHSCOPE_LLM_MODEL", "qwen-plus")
    return ChatTongyi(
        model=model,
        dashscope_api_key=api_key,
        temperature=0.7,
        max_retries=1,
    )


def analyze_photo(image_path: Path, walk_type: str) -> dict[str, Any]:
    """识图：优先 qwen-vl-plus，失败则规则兜底。"""
    fallback = _fallback_photo(walk_type)

    api_key = os.environ.get("DASHSCOPE_API_KEY")
    if not api_key:
        return fallback

    try:
        import dashscope
        from dashscope import MultiModalConversation

        dashscope.api_key = api_key
        model = os.environ.get("DASHSCOPE_VLM_MODEL", "qwen-vl-plus")
        prompt = (
            "你是 City Sprout 小芽的视觉助手。分析照片，只返回 JSON："
            '{"objects":["英文物体名"],"colors":["英文颜色名"],"summary":"一句中文，小芽口吻"}'
        )

        response = MultiModalConversation.call(
            model=model,
            messages=[
                {
                    "role": "user",
                    "content": [
                        {"image": f"file://{image_path.resolve().as_posix()}"},
                        {"text": prompt},
                    ],
                }
            ],
        )

        if response.status_code != 200:
            raise RuntimeError(response.message)

        content = response.output.choices[0].message.content
        if isinstance(content, list):
            text = "".join(
                part.get("text", "") if isinstance(part, dict) else str(part)
                for part in content
            )
        else:
            text = str(content)

        parsed = _extract_json(text)
        return {
            "objects": parsed.get("objects") or fallback["objects"],
            "colors": parsed.get("colors") or fallback["colors"],
            "summary": parsed.get("summary") or fallback["summary"],
        }
    except Exception as exc:
        print("Photo analysis fallback:", exc)
        return fallback


def _fallback_photo(walk_type: str) -> dict[str, Any]:
    colors = random.sample(COLOR_FALLBACKS, k=min(2, len(COLOR_FALLBACKS)))
    current = len(colors)
    summary = f"我看到你已经收集了 {current} 种绿色，真棒。" if walk_type == "color" else "我又记下了一个路上的瞬间。"

    return {
        "objects": ["leaf", "tree", "grass"],
        "colors": colors,
        "summary": summary,
    }


def analyze_audio(audio_path: Path, walk_type: str) -> dict[str, Any]:
    """声音 walk：用 LLM 生成标签；无 Key 时规则兜底。"""
    sounds = random.choice(SOUND_FALLBACKS)
    fallback = {
        "sounds": sounds,
        "summary": f"我好像听到了{sounds[0]}，还有{'、'.join(sounds[1:2])}。",
    }

    llm = _llm()
    if llm is None:
        return fallback

    prompt = (
        f"用户在 Sound Walk 中录了一段约 5-10 秒的户外环境音（文件 {audio_path.name}）。"
        "根据常见城市散步场景，返回 JSON："
        '{"sounds":["birdsong","traffic","human voices"],"summary":"一句中文，小芽口吻"}'
    )

    try:
        response = llm.invoke(
            [
                SystemMessage(content="你是 City Sprout 小芽，擅长描述城市里的声音。"),
                HumanMessage(content=prompt),
            ]
        )
        content = response.content if isinstance(response.content, str) else str(response.content)
        parsed = _extract_json(content)
        return {
            "sounds": parsed.get("sounds") or fallback["sounds"],
            "summary": parsed.get("summary") or fallback["summary"],
        }
    except Exception as exc:
        print("Audio analysis fallback:", exc)
        return fallback


def generate_walk_diary(session: dict[str, Any]) -> dict[str, Any]:
    """根据散步事件生成流水账与小作文。"""
    events = session.get("events", [])
    timeline = [f"{event['time']} {event['text']}" for event in events if event.get("text")]

    if not timeline:
        timeline = ["15:02 我们开始了今天的散步", "15:18 小芽记住了路上的变化"]

    fallback_essay = _fallback_essay(session)
    fallback = {
        "title": _fallback_title(session),
        "timeline": timeline,
        "essay": fallback_essay,
        "map_summary": _fallback_map_summary(session),
    }

    llm = _llm()
    if llm is None:
        return fallback

    payload = {
        "type": session.get("type"),
        "events": events,
        "photos": session.get("photos", []),
        "audios": session.get("audios", []),
        "color_progress": session.get("color_progress", {}),
    }

    prompt = (
        "根据以下散步记录，以小芽第一人称生成日记 JSON："
        '{"title":"标题","timeline":["HH:MM 事件", "..."],"essay":"150字左右小作文","map_summary":"一句话路线摘要"}\n'
        f"数据：{json.dumps(payload, ensure_ascii=False)}"
    )

    try:
        response = llm.invoke(
            [
                SystemMessage(content="你是 City Sprout 小芽，语气温柔、短句、有画面感。"),
                HumanMessage(content=prompt),
            ]
        )
        content = response.content if isinstance(response.content, str) else str(response.content)
        parsed = _extract_json(content)
        return {
            "title": parsed.get("title") or fallback["title"],
            "timeline": parsed.get("timeline") or fallback["timeline"],
            "essay": parsed.get("essay") or fallback["essay"],
            "map_summary": parsed.get("map_summary") or fallback["map_summary"],
        }
    except Exception as exc:
        print("Diary generation fallback:", exc)
        return fallback


def _fallback_title(session: dict[str, Any]) -> str:
    mapping = {
        "color": "今天我收集了很多绿色",
        "sound": "今天我听见了春天",
        "light": "今天我和阳光重新见面",
        "local": "今天我在附近发现了新线索",
    }
    return mapping.get(session.get("type"), "今天的小芽散步")


def _fallback_essay(session: dict[str, Any]) -> str:
    walk_type = session.get("type")
    if walk_type == "color":
        return (
            "今天我们一起在路边寻找绿色。每一片叶子、每一丛草丛，"
            "都像新的叶尖提醒我：真实世界一直在悄悄长大。"
        )
    if walk_type == "sound":
        return (
            "今天我们走到了有风的路口。我听见了脚步、车轮和远处的鸟鸣，"
            "城市今天不像房间里那么安静。"
        )
    return (
        "今天我们走到了有树影的路边。阳光落在我的叶子上，"
        "也落在你的鞋尖前面。谢谢你带我出去。"
    )


def _fallback_map_summary(session: dict[str, Any]) -> str:
    photo_count = len(session.get("photos", []))
    audio_count = len(session.get("audios", []))
    return f"1.8km · {photo_count} 张照片 · {audio_count} 段声音 · 阳光很足"
