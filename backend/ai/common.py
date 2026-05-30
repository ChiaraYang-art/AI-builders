"""LangChain / JSON 解析等 AI 公共工具。"""

from __future__ import annotations

import json
import os
import re
from typing import Any

from langchain_community.chat_models.tongyi import ChatTongyi

from config import LLM_MODEL


def extract_json(text: str) -> dict[str, Any]:
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


def get_chat_model(*, temperature: float = 0.7, max_retries: int = 1) -> ChatTongyi | None:
    api_key = os.environ.get("DASHSCOPE_API_KEY")
    if not api_key:
        return None

    return ChatTongyi(
        model=LLM_MODEL,
        dashscope_api_key=api_key,
        temperature=temperature,
        max_retries=max_retries,
    )
