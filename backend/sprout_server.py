"""
City Sprout / 出走小芽
文件：backend/sprout_server.py

这个 Flask 后端负责三件事：
1. 接收 AtomS3R 发来的传感器状态。
2. 用 LangChain + 百炼 qwen-plus 生成小芽对话文案（失败时规则兜底）。
3. 如果配置了百炼 API Key，就用百炼 CosyVoice TTS 把同一句话生成 MP3。

重要设计：
- POST /plant：生成 speech_short（OLED）与 speech_full（APP）。
- GET /latest：APP 轮询最新状态与 AI 文案。
- Arduino 默认仍收到 text/plain 短句，无需改动。
- 百炼 TTS 负责把 speech_full 变成声音。

运行前配置环境变量：
Windows PowerShell:
    $env:DASHSCOPE_API_KEY="你的百炼APIKey"
    python backend\\sprout_server.py

如果使用新加坡地域，可以额外设置：
    $env:DASHSCOPE_WS_URL="wss://dashscope-intl.aliyuncs.com/api-ws/v1/inference"
"""

from __future__ import annotations

import os
import threading
from datetime import datetime
from pathlib import Path
from typing import Any
from uuid import uuid4

try:
    from dotenv import load_dotenv

    _backend_dir = Path(__file__).resolve().parent
    load_dotenv(_backend_dir / ".env")
    load_dotenv(_backend_dir.parent / "deploy" / ".env")
except ImportError:
    pass

from flask import Flask, jsonify, request, send_file
from werkzeug.utils import secure_filename

from llm_speech import SproutContext, generate_speech, sound_label
from walk_ai import analyze_audio, analyze_photo, generate_walk_diary
from walk_store import (
    add_audio_result,
    add_photo_result,
    atlas_unlocked_list,
    finish_diary,
    get_active_walk_public,
    get_latest_diary,
    get_walk,
    get_walk_session,
    start_walk,
)

try:
    import dashscope
    from dashscope.audio.tts_v2 import SpeechSynthesizer
except ImportError:
    dashscope = None
    SpeechSynthesizer = None


app = Flask(__name__)


BASE_DIR = Path(__file__).resolve().parent
GENERATED_DIR = BASE_DIR / "generated"
LATEST_AUDIO_PATH = GENERATED_DIR / "latest.mp3"
WALKS_DIR = GENERATED_DIR / "walks"

TTS_MODEL = os.environ.get("DASHSCOPE_TTS_MODEL", "cosyvoice-v3-flash")
TTS_VOICE = os.environ.get("DASHSCOPE_TTS_VOICE", "longanyang")
TTS_WS_URL = os.environ.get(
    "DASHSCOPE_WS_URL",
    "wss://dashscope.aliyuncs.com/api-ws/v1/inference",
)

tts_lock = threading.Lock()
tts_last_text = ""
tts_status = "not_started"
tts_error = ""
tts_updated_at: str | None = None
_tts_running = False
_tts_pending_text: str | None = None


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
    "sound": "unknown",
    "updated_at": None,
    "audio_url": None,
    "tts_status": tts_status,
    "tts_error": "",
    "tts_updated_at": None,
    "active_walk": None,
    "walk_diary": None,
    "atlas_unlocked": atlas_unlocked_list(),
}


def sync_walk_fields() -> None:
    latest_message["active_walk"] = get_active_walk_public()
    latest_message["walk_diary"] = get_latest_diary()
    latest_message["atlas_unlocked"] = atlas_unlocked_list()


def walk_media_dir(walk_id: str) -> Path:
    path = WALKS_DIR / walk_id
    path.mkdir(parents=True, exist_ok=True)
    return path


def to_float(value: Any, default: float | None = 0.0) -> float | None:
    """把 Arduino 发来的 JSON 值转成 float；null 或错误值会回退到 default。"""
    if value is None:
        return default

    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def to_bool(value: Any) -> bool:
    """把 Arduino 发来的 true/false 或字符串 true/false 转成 Python bool。"""
    if isinstance(value, bool):
        return value

    if isinstance(value, str):
        return value.strip().lower() in {"true", "1", "yes", "y"}

    return bool(value)


def wants_json_response() -> bool:
    accept = request.headers.get("Accept", "")
    return "application/json" in accept


def parse_plant_payload(data: dict[str, Any]) -> tuple[SproutContext, dict[str, Any]]:
    """解析 POST /plant 请求体，兼容 Arduino 与 PRD 字段命名。"""
    state = str(data.get("state", "idle"))
    motion = str(data.get("motion", "still"))
    sound_state = str(data.get("sound_state", data.get("sound", "unknown")))
    place = str(data.get("place", "unknown"))

    lux = to_float(data.get("lux"), 0.0) or 0.0
    sound_level = to_float(data.get("sound_level"), 0.0) or 0.0
    sound_range = to_float(data.get("sound_range"), 0.0) or 0.0
    sound_variance = to_float(data.get("sound_variance"), 0.0) or 0.0

    env_ready = to_bool(data.get("env_ready", False))
    temperature_c = to_float(
        data.get("temperature_c", data.get("temperature")),
        None,
    )
    humidity_percent = to_float(
        data.get("humidity_percent", data.get("humidity")),
        None,
    )
    pressure_hpa = to_float(
        data.get("pressure_hpa", data.get("pressure")),
        None,
    )
    gas_resistance_ohm = to_float(data.get("gas_resistance_ohm"), None)
    iaq = to_float(data.get("iaq"), None)
    iaq_accuracy = int(to_float(data.get("iaq_accuracy"), 0) or 0)

    ctx = SproutContext(
        state=state,
        lux=lux,
        motion=motion,
        sound_state=sound_state,
        place=place,
        env_ready=env_ready,
        temperature_c=temperature_c,
        humidity_percent=humidity_percent,
        iaq=iaq,
        sound_level=sound_level,
        sound_range=sound_range,
    )

    extras = {
        "device_id": data.get("device_id"),
        "sound_variance": sound_variance,
        "pressure_hpa": pressure_hpa,
        "gas_resistance_ohm": gas_resistance_ohm,
        "iaq_accuracy": iaq_accuracy,
    }
    return ctx, extras


def set_tts_state(status: str, error: str = "") -> None:
    """集中更新 TTS 状态，方便 /latest 和网页显示。"""
    global tts_status, tts_error, tts_updated_at

    tts_status = status
    tts_error = error
    tts_updated_at = datetime.now().isoformat(timespec="seconds")

    latest_message["tts_status"] = tts_status
    latest_message["tts_error"] = tts_error
    latest_message["tts_updated_at"] = tts_updated_at
    latest_message["audio_url"] = "/audio/latest.mp3" if LATEST_AUDIO_PATH.exists() else None


def synthesize_tts_mp3(text: str) -> None:
    """
    用百炼 CosyVoice 把文本生成 MP3。

    同一时刻只跑一个 TTS，避免 Windows 上 WebSocket 并发导致 WinError 10058。
    """
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
    """串行处理 TTS 队列，避免并发 WebSocket。"""
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
    """启动后台 TTS；若已在生成，只保留最新文案。"""
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


@app.route("/plant", methods=["POST"])
def plant() -> Any:
    """
    硬件 / APP 上传传感器状态，后端用 qwen-plus 生成小芽对话。

    - Accept: application/json → 返回 PRD JSON（含 speech_short / speech_full）
    - 默认 → 返回 text/plain 短句，兼容现有 Arduino
    """
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

    if wants_json_response():
        return jsonify(response_body)

    return speech.speech_short, 200, {"Content-Type": "text/plain; charset=utf-8"}


@app.route("/latest", methods=["GET"])
def latest() -> Any:
    """给网页端和调试工具查看最新状态。"""
    latest_message["audio_url"] = "/audio/latest.mp3" if LATEST_AUDIO_PATH.exists() else None
    latest_message["tts_status"] = tts_status
    latest_message["tts_error"] = tts_error
    latest_message["tts_updated_at"] = tts_updated_at
    sync_walk_fields()
    return jsonify(latest_message)


@app.route("/audio/latest.mp3", methods=["GET"])
def latest_audio() -> Any:
    """返回最近一次 TTS 生成的 MP3。"""
    if not LATEST_AUDIO_PATH.exists():
        return "No audio generated yet.", 404

    return send_file(LATEST_AUDIO_PATH, mimetype="audio/mpeg")


@app.route("/walk/start", methods=["POST"])
def walk_start() -> Any:
    data = request.get_json(force=True, silent=True) or {}
    walk_type = str(data.get("type", "light")).strip().lower()
    session = start_walk(walk_type)
    sync_walk_fields()
    print("Walk started:", session["walk_id"], walk_type)
    return jsonify(session)


@app.route("/walk/photo", methods=["POST"])
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


@app.route("/walk/audio", methods=["POST"])
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


@app.route("/walk/diary", methods=["POST"])
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


@app.route("/walk/media/<walk_id>/<path:filename>", methods=["GET"])
def walk_media(walk_id: str, filename: str) -> Any:
    file_path = walk_media_dir(walk_id) / secure_filename(filename)
    if not file_path.exists():
        return "File not found.", 404

    return send_file(file_path)


@app.route("/", methods=["GET"])
def index() -> str:
    """简单网页：显示最新文案，并播放百炼生成的 MP3。"""
    return """
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>City Sprout Voice</title>
  <style>
    body {
      margin: 0;
      min-height: 100vh;
      display: grid;
      place-items: center;
      background: #eef4ef;
      color: #1f2a22;
      font-family: Arial, sans-serif;
    }
    main {
      width: min(680px, calc(100vw - 32px));
    }
    h1 {
      margin: 0 0 16px;
      font-size: 28px;
    }
    .panel {
      border: 1px solid #bfd0c2;
      border-radius: 8px;
      background: #ffffff;
      padding: 20px;
    }
    .speech {
      min-height: 72px;
      font-size: 30px;
      line-height: 1.25;
      margin: 16px 0;
    }
    .meta {
      color: #5d6b61;
      font-size: 14px;
      line-height: 1.7;
    }
    button {
      border: 0;
      border-radius: 6px;
      background: #2f6b43;
      color: white;
      padding: 10px 14px;
      font-size: 15px;
      cursor: pointer;
    }
    audio {
      width: 100%;
      margin-top: 12px;
    }
  </style>
</head>
<body>
  <main>
    <h1>City Sprout Voice</h1>
    <section class="panel">
      <button id="enable">Enable Audio</button>
      <div id="speech" class="speech">I am quietly waiting.</div>
      <div id="meta" class="meta">Waiting for AtomS3R...</div>
      <audio id="audio" controls></audio>
    </section>
  </main>

  <script>
    let audioEnabled = false;
    let lastSpeech = "";

    const audio = document.getElementById("audio");

    document.getElementById("enable").addEventListener("click", () => {
      audioEnabled = true;
      if (audio.src) {
        audio.play().catch(() => {});
      }
    });

    function valueOrDash(value, digits = 1) {
      if (value === null || value === undefined) {
        return "-";
      }
      return Number(value).toFixed(digits);
    }

    async function refresh() {
      const response = await fetch("/latest");
      const data = await response.json();

      document.getElementById("speech").textContent = data.speech;
      document.getElementById("meta").innerHTML =
        `state=${data.state} | lux=${valueOrDash(data.lux)} | motion=${data.motion}<br>` +
        `sound=${data.sound_state || "unknown"} | place=${data.place || "unknown"}<br>` +
        `temp=${valueOrDash(data.temperature_c)}C | hum=${valueOrDash(data.humidity_percent)}%<br>` +
        `tts=${data.tts_status || "unknown"} | updated=${data.updated_at || "never"}` +
        (data.tts_error ? `<br>tts_error=${data.tts_error}` : "");

      if (data.audio_url) {
        const audioUrl = data.audio_url + "?t=" + encodeURIComponent(data.tts_updated_at || data.updated_at || "");
        if (!audio.src.endsWith(audioUrl)) {
          audio.src = audioUrl;
        }
      }

      if (audioEnabled && data.speech && data.speech !== lastSpeech && data.audio_url && data.tts_status === "ready") {
        lastSpeech = data.speech;
        audio.play().catch(() => {});
      }
    }

    refresh();
    setInterval(refresh, 1500);
  </script>
</body>
</html>
"""


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
