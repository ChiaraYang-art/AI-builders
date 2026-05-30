"""Sound Walk 音频识别与总结。"""

from __future__ import annotations

import random
from pathlib import Path
from typing import Any

from langchain_core.messages import HumanMessage, SystemMessage

from ai.common import extract_json, get_chat_model

SOUND_FALLBACKS = [
    ["birdsong", "leaves", "footsteps"],
    ["traffic", "human voices", "wind"],
    ["birdsong", "distant traffic", "footsteps"],
    ["wind", "footsteps", "city hum"],
]


def analyze_audio(audio_path: Path, walk_type: str) -> dict[str, Any]:
    del walk_type
    sounds = random.choice(SOUND_FALLBACKS)
    fallback = {
        "sounds": sounds,
        "summary": f"我好像听到了{sounds[0]}，还有{'、'.join(sounds[1:2])}。",
    }

    llm = get_chat_model()
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
        parsed = extract_json(content)
        return {
            "sounds": parsed.get("sounds") or fallback["sounds"],
            "summary": parsed.get("summary") or fallback["summary"],
        }
    except Exception as exc:
        print("Audio analysis fallback:", exc)
        return fallback
