"""POST /plant — 硬件上传传感器状态。"""

from __future__ import annotations

from datetime import datetime
from typing import Any

from flask import Blueprint, jsonify, request

from ai.speech import generate_speech, sound_label
from ai.tts import start_tts_generation
from config import LATEST_AUDIO_PATH
from utils.response import wants_json_response
from utils.state import parse_plant_payload
from utils.storage import latest_message

bp = Blueprint("plant", __name__)


@bp.route("/plant", methods=["POST"])
def plant() -> Any:
    data = request.get_json(force=True, silent=True) or {}
    ctx, extras = parse_plant_payload(data)
    speech = generate_speech(ctx)

    latest_message.update(
        {
            "state": ctx.state,
            "lux": ctx.lux,
            "motion": ctx.motion,
            "sound_state": ctx.sound_state,
            "sound": sound_label(ctx.sound_state),
            "sound_level": ctx.sound_level,
            "sound_range": ctx.sound_range,
            "sound_variance": extras["sound_variance"],
            "place": ctx.place,
            "env_ready": ctx.env_ready,
            "temperature_c": ctx.temperature_c,
            "humidity_percent": ctx.humidity_percent,
            "pressure_hpa": extras["pressure_hpa"],
            "gas_resistance_ohm": extras["gas_resistance_ohm"],
            "iaq": ctx.iaq,
            "iaq_accuracy": extras["iaq_accuracy"],
            "speech": speech.speech_full,
            "speech_short": speech.speech_short,
            "speech_full": speech.speech_full,
            "updated_at": datetime.now().isoformat(timespec="seconds"),
        }
    )

    start_tts_generation(speech.speech_full)

    print("Received:")
    print("  state =", ctx.state)
    print("  lux =", ctx.lux)
    print("  motion =", ctx.motion)
    print("  sound_state =", ctx.sound_state)
    print("  place =", ctx.place)
    print("  temperature_c =", ctx.temperature_c)
    print("  humidity_percent =", ctx.humidity_percent)
    print("Return short:", speech.speech_short)
    print("Return full:", speech.speech_full)

    response_body = {
        "speech": speech.speech_full,
        "speech_short": speech.speech_short,
        "speech_full": speech.speech_full,
        "state": ctx.state,
        "audio_url": "/audio/latest.mp3" if LATEST_AUDIO_PATH.exists() else None,
    }

    if wants_json_response(request.headers.get("Accept")):
        return jsonify(response_body)

    return speech.speech_short, 200, {"Content-Type": "text/plain; charset=utf-8"}
