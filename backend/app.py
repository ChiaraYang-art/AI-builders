"""Flask 应用入口：注册蓝图、加载配置。"""

from __future__ import annotations

from flask import Flask

from api import register_blueprints
from config import load_env


def create_app() -> Flask:
    load_env()
    application = Flask(__name__)
    register_blueprints(application)

    from config import TTS_MODEL, TTS_VOICE

    print(f"TTS configured: model={TTS_MODEL}, voice={TTS_VOICE}")
    return application


app = create_app()


if __name__ == "__main__":
    from config import DEFAULT_PORT

    app.run(host="0.0.0.0", port=DEFAULT_PORT)
