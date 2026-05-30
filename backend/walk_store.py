"""City Sprout 散步会话：照片、录音、日记与图鉴解锁（内存存储，Demo 用）。"""

from __future__ import annotations

from copy import deepcopy
from datetime import datetime
from typing import Any
from uuid import uuid4

WALKS_DIR_NAME = "walks"
COLOR_TARGET = 5

_atlas_unlocked: set[str] = {
    "Sunny Sprout",
    "Sound Sprout",
    "City Sprout",
    "Rain Sprout",
}

_walk_sessions: dict[str, dict[str, Any]] = {}
_active_walk_id: str | None = None
_latest_diary: dict[str, Any] | None = None

_ATLAS_BY_WALK_TYPE = {
    "light": "Sunny Sprout",
    "sound": "Sound Sprout",
    "color": "Bloom Sprout",
    "local": "City Sprout",
}


def _now_text() -> str:
    return datetime.now().isoformat(timespec="seconds")


def _now_clock() -> str:
    return datetime.now().strftime("%H:%M")


def atlas_unlocked_list() -> list[str]:
    return sorted(_atlas_unlocked)


def get_latest_diary() -> dict[str, Any] | None:
    return deepcopy(_latest_diary) if _latest_diary else None


def get_active_walk_public() -> dict[str, Any] | None:
    if not _active_walk_id:
        return None

    session = _walk_sessions.get(_active_walk_id)
    if not session:
        return None

    return public_walk_view(session)


def public_walk_view(session: dict[str, Any]) -> dict[str, Any]:
    color_progress = session.get("color_progress", {})
    return {
        "walk_id": session["walk_id"],
        "type": session["type"],
        "started_at": session["started_at"],
        "color_progress": {
            "target": color_progress.get("target", COLOR_TARGET),
            "current": color_progress.get("current", 0),
            "colors": color_progress.get("colors", []),
        },
        "photos": deepcopy(session.get("photos", [])),
        "audios": deepcopy(session.get("audios", [])),
        "events": deepcopy(session.get("events", [])),
    }


def get_walk(walk_id: str) -> dict[str, Any] | None:
    session = _walk_sessions.get(walk_id)
    if not session:
        return None
    return public_walk_view(session)


def get_walk_session(walk_id: str) -> dict[str, Any] | None:
    """返回完整会话（供日记生成使用）。"""
    session = _walk_sessions.get(walk_id)
    if not session:
        return None
    return deepcopy(session)


def start_walk(walk_type: str) -> dict[str, Any]:
    global _active_walk_id

    walk_id = f"walk_{datetime.now().strftime('%Y%m%d_%H%M%S')}_{uuid4().hex[:6]}"
    session: dict[str, Any] = {
        "walk_id": walk_id,
        "type": walk_type,
        "started_at": _now_text(),
        "photos": [],
        "audios": [],
        "events": [],
        "color_progress": {
            "target": COLOR_TARGET,
            "current": 0,
            "colors": [],
        },
    }
    _walk_sessions[walk_id] = session
    _active_walk_id = walk_id

    session["events"].append(
        {
            "time": _now_clock(),
            "text": f"开始 {walk_type} walk",
            "kind": "start",
        }
    )

    return public_walk_view(session)


def add_photo_result(
    walk_id: str,
    *,
    filename: str,
    url: str,
    analysis: dict[str, Any],
) -> dict[str, Any]:
    session = _walk_sessions.get(walk_id)
    if not session:
        raise KeyError(f"walk not found: {walk_id}")

    photo_entry = {
        "filename": filename,
        "url": url,
        "objects": analysis.get("objects", []),
        "colors": analysis.get("colors", []),
        "summary": analysis.get("summary", ""),
    }
    session["photos"].append(photo_entry)

    if session["type"] == "color":
        progress = session["color_progress"]
        for color in photo_entry["colors"]:
            if color and color not in progress["colors"]:
                progress["colors"].append(color)
        progress["current"] = min(len(progress["colors"]), progress["target"])

    summary = photo_entry["summary"] or "我又拍下了一个路上的瞬间。"
    session["events"].append(
        {
            "time": _now_clock(),
            "text": summary,
            "kind": "photo",
        }
    )

    return public_walk_view(session)


def add_audio_result(
    walk_id: str,
    *,
    filename: str,
    analysis: dict[str, Any],
) -> dict[str, Any]:
    session = _walk_sessions.get(walk_id)
    if not session:
        raise KeyError(f"walk not found: {walk_id}")

    audio_entry = {
        "filename": filename,
        "sounds": analysis.get("sounds", []),
        "summary": analysis.get("summary", ""),
    }
    session["audios"].append(audio_entry)

    summary = audio_entry["summary"] or "我听见了一些城市里的声音。"
    session["events"].append(
        {
            "time": _now_clock(),
            "text": summary,
            "kind": "audio",
        }
    )

    return public_walk_view(session)


def unlock_atlas_for_type(walk_type: str) -> None:
    name = _ATLAS_BY_WALK_TYPE.get(walk_type)
    if name:
        _atlas_unlocked.add(name)

    if walk_type == "sound":
        _atlas_unlocked.add("Night Sprout")


def finish_diary(walk_id: str, diary: dict[str, Any]) -> dict[str, Any]:
    global _latest_diary, _active_walk_id

    session = _walk_sessions.get(walk_id)
    if not session:
        raise KeyError(f"walk not found: {walk_id}")

    unlock_atlas_for_type(session["type"])

    diary_payload = {
        "walk_id": walk_id,
        "type": session["type"],
        "title": diary.get("title", "今天的小芽散步"),
        "timeline": diary.get("timeline", []),
        "essay": diary.get("essay", ""),
        "generated_at": _now_text(),
        "photos": deepcopy(session.get("photos", [])),
        "audios": deepcopy(session.get("audios", [])),
        "map_summary": diary.get(
            "map_summary",
            "今天和小芽一起散步，留下了光、声音和颜色的记忆。",
        ),
    }
    _latest_diary = diary_payload

    if _active_walk_id == walk_id:
        _active_walk_id = None

    return deepcopy(diary_payload)
