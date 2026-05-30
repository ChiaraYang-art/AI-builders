"""HTTP 接口蓝图。"""

from __future__ import annotations

from flask import Flask


def register_blueprints(app: Flask) -> None:
    from api.latest import bp as latest_bp
    from api.plant import bp as plant_bp
    from api.voice_page import bp as voice_bp
    from api.walk import bp as walk_bp

    app.register_blueprint(plant_bp)
    app.register_blueprint(latest_bp)
    app.register_blueprint(walk_bp)
    app.register_blueprint(voice_bp)
