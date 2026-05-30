"""GET /latest 与 GET /audio/latest.mp3。"""

from __future__ import annotations

from typing import Any

from flask import Blueprint, jsonify, send_file

from ai.tts import get_tts_state
from config import LATEST_AUDIO_PATH
from utils.storage import latest_message, sync_walk_fields

bp = Blueprint("latest", __name__)


@bp.route("/latest", methods=["GET"])
def latest() -> Any:
    tts_status, tts_error, tts_updated_at = get_tts_state()
    latest_message["audio_url"] = "/audio/latest.mp3" if LATEST_AUDIO_PATH.exists() else None
    latest_message["tts_status"] = tts_status
    latest_message["tts_error"] = tts_error
    latest_message["tts_updated_at"] = tts_updated_at
    sync_walk_fields()
    return jsonify(latest_message)


@bp.route("/audio/latest.mp3", methods=["GET"])
def latest_audio() -> Any:
    if not LATEST_AUDIO_PATH.exists():
        return "No audio generated yet.", 404

    return send_file(LATEST_AUDIO_PATH, mimetype="audio/mpeg")
