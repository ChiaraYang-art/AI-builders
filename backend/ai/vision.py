"""Color Walk 图片识别与颜色总结。"""

from __future__ import annotations

import os
import random
from pathlib import Path
from typing import Any

from ai.common import extract_json
from config import VLM_MODEL

COLOR_FALLBACKS = [
    "light green",
    "dark green",
    "yellow green",
    "mint green",
    "forest green",
]


def analyze_photo(image_path: Path, walk_type: str) -> dict[str, Any]:
    fallback = _fallback_photo(walk_type)

    api_key = os.environ.get("DASHSCOPE_API_KEY")
    if not api_key:
        return fallback

    try:
        import dashscope
        from dashscope import MultiModalConversation

        dashscope.api_key = api_key
        prompt = (
            "你是 City Sprout 小芽的视觉助手。分析照片，只返回 JSON："
            '{"objects":["英文物体名"],"colors":["英文颜色名"],"summary":"一句中文，小芽口吻"}'
        )

        response = MultiModalConversation.call(
            model=VLM_MODEL,
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

        parsed = extract_json(text)
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
    summary = (
        f"我看到你已经收集了 {current} 种绿色，真棒。"
        if walk_type == "color"
        else "我又记下了一个路上的瞬间。"
    )

    return {
        "objects": ["leaf", "tree", "grass"],
        "colors": colors,
        "summary": summary,
    }
