"""最新状态缓存、散步会话与图鉴（内存 + JSON 持久化，Demo 用）。"""

from __future__ import annotations

import json
import os
from copy import deepcopy
from datetime import datetime
from pathlib import Path
from typing import Any
from uuid import uuid4

from config import GENERATED_DIR, LATEST_AUDIO_PATH, WALKS_DIR

COLOR_TARGET = 2
LIGHT_SUN_LUX = 300.0
LIGHT_SUN_TARGET_SECONDS = 180
GROWTH_DELTA_PER_WALK = 12

STATE_FILE = GENERATED_DIR / "sprout_state.json"

_atlas_unlocked: set[str] = {"Sunny Sprout"}

_walk_sessions: dict[str, dict[str, Any]] = {}
_active_walk_id: str | None = None
_latest_diary: dict[str, Any] | None = None
_diary_history: list[dict[str, Any]] = []
_walk_count = 0
_llm_enabled = True

_ATLAS_BY_WALK_TYPE = {
    "light": "Sunny Sprout",
    "sound": "Sound Sprout",
    "color": "Bloom Sprout",
    "local": "City Sprout",
}

latest_message: dict[str, Any] = {
    "state": "idle",
    "lux": 0.0,
    "motion": "still",
    "sound_state": "unknown",
    "sound_level": 0.0,
    "sound_range": 0.0,
    "sound_variance": 0.0,
    "place": "unknown",
    "env_ready": False,
    "temperature_c": None,
    "humidity_percent": None,
    "pressure_hpa": None,
    "gas_resistance_ohm": None,
    "iaq": None,
    "iaq_accuracy": 0,
    "speech": "I am quietly waiting.",
    "speech_short": "I am quietly waiting.",
    "speech_full": "I am quietly waiting.",
    "suggested_walk_type": None,
    "sound": "unknown",
    "updated_at": None,
    "audio_url": None,
    "tts_status": "not_started",
    "tts_error": "",
    "tts_updated_at": None,
    "active_walk": None,
    "walk_diary": None,
    "walk_count": 0,
    "diary_history": [],
    "atlas_unlocked": [],
    "llm_enabled": True,
}


def _now_text() -> str:
    return datetime.now().isoformat(timespec="seconds")


def _now_clock() -> str:
    return datetime.now().strftime("%H:%M")


def _default_light_progress() -> dict[str, Any]:
    return {
        "target_seconds": LIGHT_SUN_TARGET_SECONDS,
        "accumulated_seconds": 0,
        "complete": False,
    }


def walk_media_dir(walk_id: str) -> Path:
    path = WALKS_DIR / walk_id
    path.mkdir(parents=True, exist_ok=True)
    return path


def atlas_unlocked_list() -> list[str]:
    return sorted(_atlas_unlocked)


def diary_history_public(limit: int = 20) -> list[dict[str, Any]]:
    items = _diary_history[-limit:]
    return [deepcopy(item) for item in reversed(items)]


def save_persisted_state() -> None:
    GENERATED_DIR.mkdir(parents=True, exist_ok=True)
    payload = {
        "atlas_unlocked": sorted(_atlas_unlocked),
        "walk_count": _walk_count,
        "latest_diary": _latest_diary,
        "diary_history": _diary_history[-50:],
        "llm_enabled": _llm_enabled,
    }
    STATE_FILE.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")


def get_llm_enabled() -> bool:
    return _llm_enabled


def set_llm_enabled(enabled: bool) -> None:
    global _llm_enabled

    _llm_enabled = bool(enabled)
    latest_message["llm_enabled"] = _llm_enabled
    save_persisted_state()


def load_persisted_state() -> None:
    global _latest_diary, _walk_count, _diary_history, _llm_enabled

    if not STATE_FILE.exists():
        return

    try:
        payload = json.loads(STATE_FILE.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as exc:
        print("Load sprout_state.json failed:", exc)
        return

    _atlas_unlocked.clear()
    for name in payload.get("atlas_unlocked", ["Sunny Sprout"]):
        if name:
            _atlas_unlocked.add(str(name))

    _walk_count = int(payload.get("walk_count", 0))
    _latest_diary = payload.get("latest_diary")
    _diary_history = list(payload.get("diary_history", []))
    if "llm_enabled" in payload:
        _llm_enabled = bool(payload["llm_enabled"])


def sync_walk_fields() -> None:
    latest_message["active_walk"] = get_active_walk_public()
    latest_message["walk_diary"] = get_latest_diary()
    latest_message["walk_count"] = _walk_count
    latest_message["diary_history"] = diary_history_public()
    latest_message["atlas_unlocked"] = atlas_unlocked_list()


def init_latest_message() -> None:
    GENERATED_DIR.mkdir(parents=True, exist_ok=True)
    load_persisted_state()
    latest_message["llm_enabled"] = get_llm_enabled()
    latest_message["atlas_unlocked"] = atlas_unlocked_list()
    latest_message["walk_count"] = _walk_count
    latest_message["diary_history"] = diary_history_public()
    latest_message["audio_url"] = "/audio/latest.mp3" if LATEST_AUDIO_PATH.exists() else None
    sync_walk_fields()


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
    light_progress = session.get("light_progress", _default_light_progress())
    return {
        "walk_id": session["walk_id"],
        "type": session["type"],
        "started_at": session["started_at"],
        "color_progress": {
            "target": color_progress.get("target", COLOR_TARGET),
            "current": color_progress.get("current", 0),
            "colors": color_progress.get("colors", []),
        },
        "light_progress": {
            "target_seconds": light_progress.get("target_seconds", LIGHT_SUN_TARGET_SECONDS),
            "accumulated_seconds": light_progress.get("accumulated_seconds", 0),
            "complete": bool(light_progress.get("complete")),
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
    session = _walk_sessions.get(walk_id)
    if not session:
        return None
    return deepcopy(session)


def _walk_duration_seconds(session: dict[str, Any]) -> int:
    started = session.get("started_at")
    if not started:
        return 0
    try:
        start_dt = datetime.fromisoformat(started)
    except ValueError:
        return 0
    return max(0, int((datetime.now() - start_dt).total_seconds()))


def tick_active_walk_from_plant(lux: float, motion: str) -> None:
    if not _active_walk_id:
        return

    session = _walk_sessions.get(_active_walk_id)
    if not session or session.get("type") != "light":
        return

    progress = session.setdefault("light_progress", _default_light_progress())
    if lux >= LIGHT_SUN_LUX and motion in ("active", "walking"):
        progress["accumulated_seconds"] = min(
            progress.get("accumulated_seconds", 0) + 20,
            progress.get("target_seconds", LIGHT_SUN_TARGET_SECONDS) + 60,
        )
    progress["complete"] = progress["accumulated_seconds"] >= progress.get(
        "target_seconds", LIGHT_SUN_TARGET_SECONDS
    )


def update_light_progress(walk_id: str, sunlight_seconds: int) -> dict[str, Any]:
    session = _walk_sessions.get(walk_id)
    if not session:
        raise KeyError(f"walk not found: {walk_id}")

    progress = session.setdefault("light_progress", _default_light_progress())
    progress["accumulated_seconds"] = max(
        int(progress.get("accumulated_seconds", 0)),
        int(sunlight_seconds),
    )
    progress["complete"] = progress["accumulated_seconds"] >= progress.get(
        "target_seconds", LIGHT_SUN_TARGET_SECONDS
    )
    return public_walk_view(session)


def walk_completion_allowed(session: dict[str, Any], *, sunlight_seconds: int | None = None) -> tuple[bool, str]:
    if os.environ.get("SPROUT_DEMO_BYPASS_WALK", "").strip() == "1":
        return True, ""

    walk_type = session.get("type")
    if walk_type == "light":
        progress = session.get("light_progress", _default_light_progress())
        effective = max(
            int(progress.get("accumulated_seconds", 0)),
            int(sunlight_seconds or 0),
        )
        target = int(progress.get("target_seconds", LIGHT_SUN_TARGET_SECONDS))
        if effective < target:
            return False, f"还需要约 {max(0, target - effective)} 秒阳光"
        return True, ""

    if walk_type == "color":
        current = session.get("color_progress", {}).get("current", 0)
        if current < 1:
            return False, "至少拍 1 张路上的绿色照片"
        return True, ""

    return True, ""


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
        "light_progress": _default_light_progress(),
    }
    _walk_sessions[walk_id] = session
    _active_walk_id = walk_id

    session["events"].append(
        {
            "time": _now_clock(),
            "text": f"开始 {walk_type} 散步",
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


def add_speech_event_to_active_walk(
    text: str,
    *,
    sensor_snapshot: dict[str, Any] | None = None,
) -> None:
    """散步进行中：将 /plant 触发的小芽 speech 写入 active walk 流水账。"""
    if not _active_walk_id or not text.strip():
        return

    session = _walk_sessions.get(_active_walk_id)
    if not session:
        return

    event: dict[str, Any] = {
        "time": _now_clock(),
        "text": text.strip(),
        "kind": "speech",
    }
    if sensor_snapshot:
        event["sensor"] = deepcopy(sensor_snapshot)

    session["events"].append(event)


def unlock_atlas_for_type(walk_type: str) -> None:
    name = _ATLAS_BY_WALK_TYPE.get(walk_type)
    if name:
        _atlas_unlocked.add(name)
    if walk_type == "sound":
        _atlas_unlocked.add("Night Sprout")


def finish_diary(walk_id: str, diary: dict[str, Any]) -> dict[str, Any]:
    global _latest_diary, _active_walk_id, _walk_count

    session = _walk_sessions.get(walk_id)
    if not session:
        raise KeyError(f"walk not found: {walk_id}")

    unlock_atlas_for_type(session["type"])
    _walk_count += 1

    light_progress = session.get("light_progress", _default_light_progress())
    duration_seconds = _walk_duration_seconds(session)

    diary_payload = {
        "walk_id": walk_id,
        "type": session["type"],
        "walk_number": _walk_count,
        "title": diary.get("title", "今天的小芽散步"),
        "timeline": diary.get("timeline", []),
        "essay": diary.get("essay", ""),
        "share_caption": diary.get(
            "share_caption",
            f"今天带小芽第 {_walk_count} 次出门，它写了一篇日记。",
        ),
        "generated_at": _now_text(),
        "photos": deepcopy(session.get("photos", [])),
        "audios": deepcopy(session.get("audios", [])),
        "map_summary": diary.get(
            "map_summary",
            "今天和小芽一起散步，留下了光、声音和颜色的记忆。",
        ),
        "growth_delta": GROWTH_DELTA_PER_WALK,
        "sunlight_seconds": int(light_progress.get("accumulated_seconds", 0)),
        "duration_seconds": duration_seconds,
        "discovered_elements": diary.get("discovered_elements", []),
    }
    _latest_diary = diary_payload
    _diary_history.append(deepcopy(diary_payload))
    save_persisted_state()

    if _active_walk_id == walk_id:
        _active_walk_id = None

    return deepcopy(diary_payload)


init_latest_message()
