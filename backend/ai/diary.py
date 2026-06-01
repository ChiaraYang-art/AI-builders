"""散步日记（timeline + essay）生成。"""

from __future__ import annotations

import json
from typing import Any

from langchain_core.messages import HumanMessage, SystemMessage

from ai.common import extract_json, get_chat_model


def generate_walk_diary(
    session: dict[str, Any],
    latest_snapshot: dict[str, Any] | None = None,
) -> dict[str, Any]:
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

    llm = get_chat_model()
    if llm is None:
        return fallback

    payload: dict[str, Any] = {
        "type": session.get("type"),
        "events": events,
        "photos": session.get("photos", []),
        "audios": session.get("audios", []),
        "color_progress": session.get("color_progress", {}),
        "light_progress": session.get("light_progress", {}),
    }
    if latest_snapshot:
        payload["latest_snapshot"] = latest_snapshot

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
        parsed = extract_json(content)
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
