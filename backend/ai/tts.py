"""百炼 CosyVoice TTS，生成 latest.mp3。"""

from __future__ import annotations

import os
import threading
from datetime import datetime

from config import GENERATED_DIR, LATEST_AUDIO_PATH, TTS_MODEL, TTS_VOICE, TTS_WS_URL
from utils.storage import latest_message

try:
    import dashscope
    from dashscope.audio.tts_v2 import SpeechSynthesizer
except ImportError:
    dashscope = None
    SpeechSynthesizer = None

tts_lock = threading.Lock()
tts_last_text = ""
tts_status = "not_started"
tts_error = ""
tts_updated_at: str | None = None
_tts_running = False
_tts_pending_text: str | None = None


def get_tts_state() -> tuple[str, str, str | None]:
    return tts_status, tts_error, tts_updated_at


def set_tts_state(status: str, error: str = "") -> None:
    global tts_status, tts_error, tts_updated_at

    tts_status = status
    tts_error = error
    tts_updated_at = datetime.now().isoformat(timespec="seconds")

    latest_message["tts_status"] = tts_status
    latest_message["tts_error"] = tts_error
    latest_message["tts_updated_at"] = tts_updated_at
    latest_message["audio_url"] = "/audio/latest.mp3" if LATEST_AUDIO_PATH.exists() else None


def synthesize_tts_mp3(text: str) -> None:
    global tts_last_text

    api_key = os.environ.get("DASHSCOPE_API_KEY")

    if not api_key:
        set_tts_state("skipped", "DASHSCOPE_API_KEY is not set.")
        return

    if dashscope is None or SpeechSynthesizer is None:
        set_tts_state("skipped", "dashscope package is not installed.")
        return

    if text == tts_last_text and LATEST_AUDIO_PATH.exists():
        set_tts_state("ready")
        return

    set_tts_state("generating")

    last_exc: Exception | None = None
    for attempt in range(2):
        try:
            dashscope.api_key = api_key
            dashscope.base_websocket_api_url = TTS_WS_URL

            synthesizer = SpeechSynthesizer(model=TTS_MODEL, voice=TTS_VOICE)
            audio = synthesizer.call(text)

            if not audio:
                raise RuntimeError("TTS returned empty audio.")

            GENERATED_DIR.mkdir(parents=True, exist_ok=True)
            LATEST_AUDIO_PATH.write_bytes(audio)

            tts_last_text = text
            set_tts_state("ready")

            print("TTS ready:")
            print("  text =", text)
            print("  request_id =", synthesizer.get_last_request_id())
            print("  first_package_delay =", synthesizer.get_first_package_delay())
            return
        except Exception as exc:
            last_exc = exc
            print(f"TTS failed (attempt {attempt + 1}/2):", exc)

    set_tts_state("error", str(last_exc) if last_exc else "TTS failed.")


def _tts_worker() -> None:
    global _tts_running, _tts_pending_text

    while True:
        with tts_lock:
            text = _tts_pending_text
            _tts_pending_text = None
            if not text:
                _tts_running = False
                return

        synthesize_tts_mp3(text)


def start_tts_generation(text: str) -> None:
    global _tts_running, _tts_pending_text

    if text == tts_last_text and LATEST_AUDIO_PATH.exists():
        set_tts_state("ready")
        return

    with tts_lock:
        _tts_pending_text = text
        if _tts_running:
            return
        _tts_running = True

    thread = threading.Thread(target=_tts_worker, daemon=True)
    thread.start()
