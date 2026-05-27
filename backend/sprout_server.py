"""
City Sprout / 出走小芽
文件：backend/sprout_server.py

这是给 AtomS3R 调用的 Flask 后端。

当前版本仍然是“规则版 AI”：
1. Arduino 把小芽状态、光照、运动、声音、地点判断、ENV-Pro 环境数据发到 /plant。
2. Flask 根据这些数据生成一句很短的植物文案。
3. Flask 返回纯文本，Arduino 收到后显示在 OLED 上。
4. 浏览器打开服务器首页时，可以看到最新文案，并用浏览器 speechSynthesis 朗读。

以后接入阿里云百炼或其他大模型时，主要替换 generate_plant_speech()。
其他接口结构可以继续沿用。
"""

from __future__ import annotations

from datetime import datetime
from typing import Any

from flask import Flask, jsonify, request


app = Flask(__name__)


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
}


def to_float(value: Any, default: float | None = 0.0) -> float | None:
    """
    把 Arduino 发来的 JSON 值转成 float。

    为什么要单独写这个函数：
    Arduino 有时会发 null，例如 ENV-Pro 还没有第一组数据时。
    Python 的 float(None) 会报错，所以这里统一处理。
    """
    if value is None:
        return default

    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def to_bool(value: Any) -> bool:
    """
    把 Arduino 发来的 env_ready 转成 bool。

    Arduino JSON 里一般会发 true / false。
    但为了兼容手动测试，也接受 "true"、"1"、"yes"。
    """
    if isinstance(value, bool):
        return value

    if isinstance(value, str):
        return value.strip().lower() in {"true", "1", "yes", "y"}

    return bool(value)


def generate_plant_speech(
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
    根据硬件状态生成一句植物说的话。

    当前是规则版，目标不是写得很聪明，而是先保证链路稳定：
    传感器 -> Arduino 状态 -> Flask -> OLED 文案。

    文案要求：
    - 英文。
    - 尽量短，因为 OLED 很小。
    - 像植物在表达感受，不像健康打卡提醒。
    - 不责备用户。
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


@app.route("/plant", methods=["POST"])
def plant() -> tuple[str, int, dict[str, str]]:
    """
    Arduino 调用的接口。

    新 PaHUB 主程序会发送类似：
    {
      "state": "walking",
      "lux": 840.2,
      "motion": "walking",
      "sound_state": "dynamic",
      "sound_level": 0.031,
      "sound_range": 0.024,
      "sound_variance": 0.00012,
      "place": "outside",
      "env_ready": true,
      "temperature_c": 26.5,
      "humidity_percent": 61.2,
      "pressure_hpa": 1008.4,
      "gas_resistance_ohm": 23500,
      "iaq": 72,
      "iaq_accuracy": 1
    }

    返回值是纯文本，不是 JSON。
    这样 Arduino 端处理最简单，直接把 response 显示到 OLED。
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

    speech = generate_plant_speech(
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

    print("Received:")
    print("  state =", state)
    print("  lux =", lux)
    print("  motion =", motion)
    print("  sound_state =", sound_state)
    print("  sound_level =", sound_level)
    print("  sound_range =", sound_range)
    print("  sound_variance =", sound_variance)
    print("  place =", place)
    print("  env_ready =", env_ready)
    print("  temperature_c =", temperature_c)
    print("  humidity_percent =", humidity_percent)
    print("  pressure_hpa =", pressure_hpa)
    print("  gas_resistance_ohm =", gas_resistance_ohm)
    print("  iaq =", iaq)
    print("  iaq_accuracy =", iaq_accuracy)
    print("Return:", speech)

    return speech, 200, {"Content-Type": "text/plain; charset=utf-8"}


@app.route("/latest", methods=["GET"])
def latest() -> Any:
    """给网页端查看最新小芽状态。"""
    return jsonify(latest_message)


@app.route("/", methods=["GET"])
def index() -> str:
    """
    简单网页 TTS 页面。

    打开 http://服务器IP:5000/
    点 Enable Voice 后，浏览器会朗读最新植物文案。
    """
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
  </style>
</head>
<body>
  <main>
    <h1>City Sprout Voice</h1>
    <section class="panel">
      <button id="enable">Enable Voice</button>
      <div id="speech" class="speech">I am quietly waiting.</div>
      <div id="meta" class="meta">Waiting for AtomS3R...</div>
    </section>
  </main>

  <script>
    let voiceEnabled = false;
    let lastSpeech = "";

    document.getElementById("enable").addEventListener("click", () => {
      voiceEnabled = true;
      speak(document.getElementById("speech").textContent);
    });

    function valueOrDash(value, digits = 1) {
      if (value === null || value === undefined) {
        return "-";
      }
      return Number(value).toFixed(digits);
    }

    function speak(text) {
      if (!voiceEnabled || !("speechSynthesis" in window)) {
        return;
      }

      window.speechSynthesis.cancel();
      const utterance = new SpeechSynthesisUtterance(text);
      utterance.lang = "en-US";
      utterance.rate = 0.9;
      utterance.pitch = 1.15;
      window.speechSynthesis.speak(utterance);
    }

    async function refresh() {
      const response = await fetch("/latest");
      const data = await response.json();

      document.getElementById("speech").textContent = data.speech;
      document.getElementById("meta").innerHTML =
        `state=${data.state} | lux=${valueOrDash(data.lux)} | motion=${data.motion}<br>` +
        `sound=${data.sound_state || "unknown"} | place=${data.place || "unknown"}<br>` +
        `temp=${valueOrDash(data.temperature_c)}C | hum=${valueOrDash(data.humidity_percent)}% | iaq=${valueOrDash(data.iaq, 0)}<br>` +
        `updated=${data.updated_at || "never"}`;

      if (data.speech && data.speech !== lastSpeech) {
        lastSpeech = data.speech;
        speak(data.speech);
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
