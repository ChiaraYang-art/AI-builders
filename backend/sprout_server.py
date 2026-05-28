"""
City Sprout / 出走小芽
文件：backend/sprout_server.py

这个 Flask 后端负责三件事：
1. 接收 AtomS3R 发来的传感器状态。
2. 用规则生成一条短文案，并把文本返回给 Arduino 显示在 OLED。
3. 如果配置了百炼 API Key，就用百炼 CosyVoice TTS 把同一句话生成 MP3。

重要设计：
- 现在 OLED 文案先不用大模型生成，保持稳定、可控、短句。
- 百炼当前只负责“语音合成”，也就是把已有文案变成声音。
- Arduino 端仍然只需要 POST /plant，不需要知道 API Key。
- 未来硬件播放声音时，可以让 AtomS3R 下载 /audio/latest.mp3 再交给 Atomic Voice Base 播放。

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

from flask import Flask, jsonify, request, send_file

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
    "updated_at": None,
    "audio_url": None,
    "tts_status": tts_status,
    "tts_error": "",
    "tts_updated_at": None,
}


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


def generate_rule_speech(
    *,
    state: str,
    lux: float,
    motion: str,
    sound_state: str = "unknown",
    place: str = "unknown",
    env_ready: bool = False,
    temperature_c: float | None = None,
    humidity_percent: float | None = None,
    iaq: float | None = None,
) -> str:
    """
    规则版文案生成。

    现在先不让大模型写 OLED 文案，因为 OLED 很小，稳定短句更重要。
    后续如果要让大模型写日记、长反馈，可以新开接口，不影响这个硬件闭环。
    """
    state = (state or "idle").strip().lower()
    motion = (motion or "still").strip().lower()
    sound_state = (sound_state or "unknown").strip().lower()
    place = (place or "unknown").strip().lower()

    if place == "outside":
        if sound_state == "dynamic":
            return "The city sounds wider here."
        if lux >= 800:
            return "The outside light found us."
        return "The air feels open today."

    if state == "walking" or motion == "walking":
        if sound_state == "dynamic":
            return "I hear the world moving."
        return "Are we going outside?"

    if env_ready and temperature_c is not None and temperature_c >= 30:
        return "The air feels warm today."

    if env_ready and humidity_percent is not None and humidity_percent >= 75:
        return "The air feels soft and wet."

    if env_ready and iaq is not None and iaq >= 150:
        return "This air feels a little heavy."

    if place == "indoor":
        return "It feels quiet in here."

    if state == "wilted":
        return "I only saw screen light today."

    if state == "need_sun":
        return "Please take me to real sun."

    if state == "sunlight":
        return "Sunlight found. I feel alive."

    if lux < 50:
        return "I need a little sun."

    if lux < 300:
        return "Can we find real sun?"

    return "I am quietly waiting."


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

    这个函数会在后台线程运行，避免 Arduino 等 TTS 太久。
    """
    global tts_last_text

    with tts_lock:
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

        try:
            dashscope.api_key = api_key
            dashscope.base_websocket_api_url = TTS_WS_URL

            synthesizer = SpeechSynthesizer(model=TTS_MODEL, voice=TTS_VOICE)
            audio = synthesizer.call(text)

            GENERATED_DIR.mkdir(parents=True, exist_ok=True)
            LATEST_AUDIO_PATH.write_bytes(audio)

            tts_last_text = text
            set_tts_state("ready")

            print("TTS ready:")
            print("  text =", text)
            print("  request_id =", synthesizer.get_last_request_id())
            print("  first_package_delay =", synthesizer.get_first_package_delay())
        except Exception as exc:
            set_tts_state("error", str(exc))
            print("TTS failed:", exc)


def start_tts_generation(text: str) -> None:
    """启动后台 TTS 生成。"""
    thread = threading.Thread(target=synthesize_tts_mp3, args=(text,), daemon=True)
    thread.start()


@app.route("/plant", methods=["POST"])
def plant() -> tuple[str, int, dict[str, str]]:
    """
    Arduino 调用的接口。

    返回值仍然是纯文本，Arduino 不需要改。
    后端会在后台把这句文本变成 latest.mp3。
    """
    data = request.get_json(force=True, silent=True) or {}

    state = str(data.get("state", "idle"))
    motion = str(data.get("motion", "still"))
    sound_state = str(data.get("sound_state", "unknown"))
    place = str(data.get("place", "unknown"))

    lux = to_float(data.get("lux"), 0.0) or 0.0
    sound_level = to_float(data.get("sound_level"), 0.0) or 0.0
    sound_range = to_float(data.get("sound_range"), 0.0) or 0.0
    sound_variance = to_float(data.get("sound_variance"), 0.0) or 0.0

    env_ready = to_bool(data.get("env_ready", False))
    temperature_c = to_float(data.get("temperature_c"), None)
    humidity_percent = to_float(data.get("humidity_percent"), None)
    pressure_hpa = to_float(data.get("pressure_hpa"), None)
    gas_resistance_ohm = to_float(data.get("gas_resistance_ohm"), None)
    iaq = to_float(data.get("iaq"), None)
    iaq_accuracy = int(to_float(data.get("iaq_accuracy"), 0) or 0)

    speech = generate_rule_speech(
        state=state,
        lux=lux,
        motion=motion,
        sound_state=sound_state,
        place=place,
        env_ready=env_ready,
        temperature_c=temperature_c,
        humidity_percent=humidity_percent,
        iaq=iaq,
    )

    latest_message.update(
        {
            "state": state,
            "lux": lux,
            "motion": motion,
            "sound_state": sound_state,
            "sound_level": sound_level,
            "sound_range": sound_range,
            "sound_variance": sound_variance,
            "place": place,
            "env_ready": env_ready,
            "temperature_c": temperature_c,
            "humidity_percent": humidity_percent,
            "pressure_hpa": pressure_hpa,
            "gas_resistance_ohm": gas_resistance_ohm,
            "iaq": iaq,
            "iaq_accuracy": iaq_accuracy,
            "speech": speech,
            "updated_at": datetime.now().isoformat(timespec="seconds"),
        }
    )

    start_tts_generation(speech)

    print("Received:")
    print("  state =", state)
    print("  lux =", lux)
    print("  motion =", motion)
    print("  sound_state =", sound_state)
    print("  place =", place)
    print("  temperature_c =", temperature_c)
    print("  humidity_percent =", humidity_percent)
    print("Return:", speech)

    return speech, 200, {"Content-Type": "text/plain; charset=utf-8"}


@app.route("/latest", methods=["GET"])
def latest() -> Any:
    """给网页端和调试工具查看最新状态。"""
    latest_message["audio_url"] = "/audio/latest.mp3" if LATEST_AUDIO_PATH.exists() else None
    latest_message["tts_status"] = tts_status
    latest_message["tts_error"] = tts_error
    latest_message["tts_updated_at"] = tts_updated_at
    return jsonify(latest_message)


@app.route("/audio/latest.mp3", methods=["GET"])
def latest_audio() -> Any:
    """返回最近一次 TTS 生成的 MP3。"""
    if not LATEST_AUDIO_PATH.exists():
        return "No audio generated yet.", 404

    return send_file(LATEST_AUDIO_PATH, mimetype="audio/mpeg")


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
