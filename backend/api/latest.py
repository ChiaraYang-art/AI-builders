"""GET /latest 与 GET /audio/latest.mp3。"""

from __future__ import annotations

from typing import Any

from flask import Blueprint, jsonify, send_file, send_from_directory

from ai.tts import get_tts_state
from config import AUDIO_LIBRARY_DIR, LATEST_AUDIO_PATH, TTS_MODEL, TTS_VOICE
from utils.storage import latest_message, sync_walk_fields

bp = Blueprint("latest", __name__)


@bp.route("/latest", methods=["GET"])
def latest() -> Any:
    tts_status, tts_error, tts_updated_at = get_tts_state()
    latest_message["audio_url"] = "/audio/latest.mp3" if LATEST_AUDIO_PATH.exists() else None
    latest_message["tts_status"] = tts_status
    latest_message["tts_error"] = tts_error
    latest_message["tts_updated_at"] = tts_updated_at
    latest_message["tts_voice"] = TTS_VOICE
    latest_message["tts_model"] = TTS_MODEL
    sync_walk_fields()
    return jsonify(latest_message)


@bp.route("/audio/latest.mp3", methods=["GET"])
def latest_audio() -> Any:
    if not LATEST_AUDIO_PATH.exists():
        return "No audio generated yet.", 404

    return send_file(LATEST_AUDIO_PATH, mimetype="audio/mpeg")


@bp.route("/audio/library/<path:filename>", methods=["GET"])
def library_audio(filename: str) -> Any:
    if not AUDIO_LIBRARY_DIR.exists():
        return "Audio library not found.", 404

    return send_from_directory(AUDIO_LIBRARY_DIR, filename, mimetype="audio/mpeg")
