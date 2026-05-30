"""散步相关接口：/walk/*。"""

from __future__ import annotations

from pathlib import Path
from typing import Any
from uuid import uuid4

from flask import Blueprint, jsonify, request, send_file
from werkzeug.utils import secure_filename

from ai.audio import analyze_audio
from ai.diary import generate_walk_diary
from ai.vision import analyze_photo
from utils.storage import (
    add_audio_result,
    add_photo_result,
    finish_diary,
    get_walk,
    get_walk_session,
    start_walk,
    sync_walk_fields,
    walk_media_dir,
)

bp = Blueprint("walk", __name__)


@bp.route("/walk/start", methods=["POST"])
def walk_start() -> Any:
    data = request.get_json(force=True, silent=True) or {}
    walk_type = str(data.get("type", "light")).strip().lower()
    session = start_walk(walk_type)
    sync_walk_fields()
    print("Walk started:", session["walk_id"], walk_type)
    return jsonify(session)


@bp.route("/walk/photo", methods=["POST"])
def walk_photo() -> Any:
    walk_id = str(request.form.get("walk_id", "")).strip()
    image = request.files.get("image")

    if not walk_id or image is None or not image.filename:
        return jsonify({"error": "walk_id and image are required."}), 400

    session = get_walk(walk_id)
    if session is None:
        return jsonify({"error": f"walk not found: {walk_id}"}), 404

    suffix = Path(secure_filename(image.filename)).suffix or ".jpg"
    filename = f"photo_{uuid4().hex[:8]}{suffix}"
    save_path = walk_media_dir(walk_id) / filename
    image.save(save_path)

    analysis = analyze_photo(save_path, session["type"])
    media_url = f"/walk/media/{walk_id}/{filename}"
    updated = add_photo_result(
        walk_id,
        filename=filename,
        url=media_url,
        analysis=analysis,
    )
    sync_walk_fields()

    return jsonify(
        {
            **analysis,
            "walk_id": walk_id,
            "url": media_url,
            "progress": updated["color_progress"],
            "active_walk": updated,
        }
    )


@bp.route("/walk/audio", methods=["POST"])
def walk_audio() -> Any:
    walk_id = str(request.form.get("walk_id", "")).strip()
    audio = request.files.get("audio")

    if not walk_id or audio is None or not audio.filename:
        return jsonify({"error": "walk_id and audio are required."}), 400

    session = get_walk(walk_id)
    if session is None:
        return jsonify({"error": f"walk not found: {walk_id}"}), 404

    suffix = Path(secure_filename(audio.filename)).suffix or ".webm"
    filename = f"audio_{uuid4().hex[:8]}{suffix}"
    save_path = walk_media_dir(walk_id) / filename
    audio.save(save_path)

    analysis = analyze_audio(save_path, session["type"])
    updated = add_audio_result(walk_id, filename=filename, analysis=analysis)
    sync_walk_fields()

    return jsonify(
        {
            **analysis,
            "walk_id": walk_id,
            "active_walk": updated,
        }
    )


@bp.route("/walk/diary", methods=["POST"])
def walk_diary() -> Any:
    data = request.get_json(force=True, silent=True) or {}
    walk_id = str(data.get("walk_id", "")).strip()

    if not walk_id:
        return jsonify({"error": "walk_id is required."}), 400

    session = get_walk_session(walk_id)
    if session is None:
        return jsonify({"error": f"walk not found: {walk_id}"}), 404

    diary = generate_walk_diary(session)
    payload = finish_diary(walk_id, diary)
    sync_walk_fields()
    print("Walk diary ready:", walk_id, payload.get("title"))
    return jsonify(payload)


@bp.route("/walk/media/<walk_id>/<path:filename>", methods=["GET"])
def walk_media(walk_id: str, filename: str) -> Any:
    file_path = walk_media_dir(walk_id) / secure_filename(filename)
    if not file_path.exists():
        return "File not found.", 404

    return send_file(file_path)
