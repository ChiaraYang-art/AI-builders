"""环境变量、路径与全局配置。"""

from __future__ import annotations

import os
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent
GENERATED_DIR = BASE_DIR / "generated"
LATEST_AUDIO_PATH = GENERATED_DIR / "latest.mp3"
WALKS_DIR = GENERATED_DIR / "walks"
PROMPTS_DIR = BASE_DIR / "prompts"

DEFAULT_PORT = int(os.environ.get("SPROUT_PORT", "5000"))

TTS_MODEL = os.environ.get("DASHSCOPE_TTS_MODEL", "cosyvoice-v3-flash")
TTS_VOICE = os.environ.get("DASHSCOPE_TTS_VOICE", "longanyang")
TTS_WS_URL = os.environ.get(
    "DASHSCOPE_WS_URL",
    "wss://dashscope.aliyuncs.com/api-ws/v1/inference",
)

LLM_MODEL = os.environ.get("DASHSCOPE_LLM_MODEL", "qwen-plus")
VLM_MODEL = os.environ.get("DASHSCOPE_VLM_MODEL", "qwen-vl-plus")


def load_env() -> None:
    try:
        from dotenv import load_dotenv

        load_dotenv(BASE_DIR / ".env")
        load_dotenv(BASE_DIR.parent / "deploy" / ".env")
    except ImportError:
        pass
