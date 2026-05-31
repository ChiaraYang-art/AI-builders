"""演示设置：网页端控制是否调用大模型。"""

from __future__ import annotations

from typing import Any

from flask import Blueprint, jsonify, request

from utils.storage import get_llm_enabled, set_llm_enabled

bp = Blueprint("settings", __name__)


@bp.route("/settings/llm", methods=["GET", "POST"])
def llm_settings() -> Any:
    if request.method == "GET":
        return jsonify({"llm_enabled": get_llm_enabled()})

    data = request.get_json(force=True, silent=True) or {}
    if "llm_enabled" not in data:
        return jsonify({"error": "missing llm_enabled"}), 400

    set_llm_enabled(bool(data["llm_enabled"]))
    print("LLM demo setting:", "enabled" if get_llm_enabled() else "disabled")
    return jsonify({"llm_enabled": get_llm_enabled()})
