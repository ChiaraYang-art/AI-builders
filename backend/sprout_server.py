"""
City Sprout / 出走小芽
文件：backend/sprout_server.py

电脑端 Flask 后端。

当前功能：
1. 接收 AtomS3R 发来的 state / lux / motion。
2. 兼容声音实验版发来的 sound_level / sound_state / place。
3. 用规则生成一句短植物语言。
4. 提供网页 TTS 页面，用电脑扬声器临时模拟植物说话。

后续接大模型时，主要替换 generate_plant_speech()。
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
    "place": "unknown",
    "speech": "I am quietly waiting.",
    "updated_at": None,
}


def generate_plant_speech(
    state: str,
    lux: float,
    motion: str,
    sound_state: str = "unknown",
    place: str = "unknown",
) -> str:
    """
    根据硬件状态生成一句短句。

    现在是规则版，不调用真正的大模型。
    输出要短，因为之后会显示在小 OLED 上。
    """
    state = (state or "idle").strip().lower()
    motion = (motion or "still").strip().lower()
    sound_state = (sound_state or "unknown").strip().lower()
    place = (place or "unknown").strip().lower()

    if place == "outside":
        if sound_state == "dynamic":
            return "The city sounds wider here."
        return "The outside light found us."

    if state == "walking" or motion == "walking":
        if sound_state == "dynamic":
            return "I hear the world moving."
        return "Are we going outside?"

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

    基础版请求示例：
        {"state": "wilted", "lux": 18.5, "motion": "still"}

    声音实验版请求示例：
        {
          "state": "walking",
          "lux": 840.2,
          "motion": "walking",
          "sound_level": 0.031,
          "sound_range": 0.024,
          "sound_variance": 0.00012,
          "sound_state": "dynamic",
          "place": "outside"
        }
    """
    data = request.get_json(force=True, silent=True) or {}

    state = str(data.get("state", "idle"))
    motion = str(data.get("motion", "still"))
    sound_state = str(data.get("sound_state", "unknown"))
    place = str(data.get("place", "unknown"))

    try:
      lux = float(data.get("lux", 0))
    except (TypeError, ValueError):
      lux = 0.0

    try:
      sound_level = float(data.get("sound_level", 0))
    except (TypeError, ValueError):
      sound_level = 0.0

    try:
      sound_range = float(data.get("sound_range", 0))
    except (TypeError, ValueError):
      sound_range = 0.0

    try:
      sound_variance = float(data.get("sound_variance", 0))
    except (TypeError, ValueError):
      sound_variance = 0.0

    speech = generate_plant_speech(
        state=state,
        lux=lux,
        motion=motion,
        sound_state=sound_state,
        place=place,
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
    print("Return:", speech)

    return speech, 200, {"Content-Type": "text/plain; charset=utf-8"}


@app.route("/latest", methods=["GET"])
def latest() -> Any:
    return jsonify(latest_message)


@app.route("/", methods=["GET"])
def index() -> str:
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
      width: min(620px, calc(100vw - 32px));
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
        `state=${data.state} | lux=${Number(data.lux).toFixed(1)} | motion=${data.motion}<br>` +
        `sound=${data.sound_state || "unknown"} | place=${data.place || "unknown"} | updated=${data.updated_at || "never"}`;

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
