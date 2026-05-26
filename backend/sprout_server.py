"""
City Sprout / 出走小芽
文件：backend/sprout_server.py

这个文件是电脑端 Flask 后端。

当前作用：
1. 接收 AtomS3R 通过 HTTP POST 发来的 JSON 数据。
2. JSON 里包含 state / lux / motion。
3. 根据规则生成一句“植物语言”。
4. 把这句话用纯文本返回给 AtomS3R。
5. 额外提供一个网页页面，用浏览器 speechSynthesis 临时模拟 TTS 语音。

为什么现在先用规则，不直接接大模型：
硬件 Demo 第一阶段最重要的是把链路跑通：
传感器 -> 状态判断 -> 后端文案 -> 屏幕表达。
等这个链路稳定后，再把 generate_plant_speech() 替换成真正的大模型调用。
"""

from __future__ import annotations

from datetime import datetime
from typing import Any

from flask import Flask, jsonify, request


# 创建 Flask 应用。
# 你可以把 app 理解成“电脑上运行的小型网页服务器”。
app = Flask(__name__)


# 这里保存最近一次植物说的话。
# 网页 TTS 页面会定时读取这个变量，然后在电脑浏览器里显示和朗读。
latest_message: dict[str, Any] = {
    "state": "idle",
    "lux": 0.0,
    "motion": "still",
    "speech": "I am quietly waiting.",
    "updated_at": None,
}


def generate_plant_speech(state: str, lux: float, motion: str) -> str:
    """
    根据植物状态生成一句短句。

    参数说明：
    state:
        AtomS3R 发来的植物状态，例如 "wilted"、"need_sun"。
    lux:
        DLight 光照传感器读到的数值，单位是 lux。
    motion:
        动作状态。当前 Arduino 端还没有接 IMU，所以通常是 "still"。
        未来接入 IMU 后可以传 "walking"、"picked_up" 等。

    返回：
        一句适合显示在小 OLED 上的英文短句。

    未来接大模型的位置：
        后续可以把本函数内部的 if 规则替换成 OpenAI / 其他 LLM API 调用。
        Prompt 建议：
        - You are a small AI plant that wants to go outside.
        - Speak gently.
        - Do not blame the user.
        - Do not sound like a health app.
        - Do not command the user.
        - Keep the sentence under 12 words.
        - It should fit on a tiny OLED screen.
    """
    state = (state or "idle").strip().lower()
    motion = (motion or "still").strip().lower()

    # 动作状态优先级较高。
    # 如果未来 IMU 判断正在走动，就算光照还没变，也可以先回应“我们要出门吗？”
    if state == "walking" or motion == "walking":
        return "Are we going outside?"

    if state == "wilted":
        return "I only saw screen light today."

    if state == "need_sun":
        return "Please take me to real sun."

    if state == "sunlight":
        return "Sunlight found. I feel alive."

    # 如果 Arduino 端没有明确传状态，后端也可以根据 lux 做一个兜底判断。
    if lux < 50:
        return "I need a little sun."

    if lux < 300:
        return "Can we find real sun?"

    return "I am quietly waiting."


@app.route("/plant", methods=["POST"])
def plant() -> tuple[str, int, dict[str, str]]:
    """
    AtomS3R 调用的主接口。

    请求方式：
        POST /plant

    请求 JSON 示例：
        {
          "state": "wilted",
          "lux": 18.5,
          "motion": "still"
        }

    返回：
        纯文本短句，例如：
        I only saw screen light today.

    为什么返回纯文本而不是 JSON：
        Arduino / ESP32 端处理纯文本最简单。
        第一版先降低解析复杂度，保证硬件链路稳定。
    """
    data = request.get_json(force=True, silent=True) or {}

    state = str(data.get("state", "idle"))
    motion = str(data.get("motion", "still"))

    try:
        lux = float(data.get("lux", 0))
    except (TypeError, ValueError):
        lux = 0.0

    speech = generate_plant_speech(state=state, lux=lux, motion=motion)

    latest_message.update(
        {
            "state": state,
            "lux": lux,
            "motion": motion,
            "speech": speech,
            "updated_at": datetime.now().isoformat(timespec="seconds"),
        }
    )

    print("Received:")
    print("  state =", state)
    print("  lux =", lux)
    print("  motion =", motion)
    print("Return:", speech)

    # 第三个返回值是 HTTP header。
    # text/plain 告诉客户端：返回内容就是普通文本。
    return speech, 200, {"Content-Type": "text/plain; charset=utf-8"}


@app.route("/latest", methods=["GET"])
def latest() -> Any:
    """
    给网页 TTS 页面使用的接口。

    浏览器会定时 GET /latest，读取最近一次植物说的话。
    这样电脑扬声器就可以临时充当“植物声音”。
    """
    return jsonify(latest_message)


@app.route("/", methods=["GET"])
def index() -> str:
    """
    一个极简网页 TTS 页面。

    使用方式：
    1. 先运行 python backend/sprout_server.py。
    2. 用浏览器打开 http://127.0.0.1:5000/。
    3. 让 AtomS3R 继续 POST /plant。
    4. 网页会显示最新文案，并用浏览器 speechSynthesis 朗读。

    注意：
    浏览器通常要求用户先点击一次按钮，才允许网页自动播放语音。
    所以页面里提供了 Enable Voice 按钮。
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
      width: min(560px, calc(100vw - 32px));
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
      line-height: 1.6;
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
      document.getElementById("meta").textContent =
        `state=${data.state} | lux=${Number(data.lux).toFixed(1)} | motion=${data.motion} | updated=${data.updated_at || "never"}`;

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
    # host="0.0.0.0" 的意思是：
    # 允许同一 Wi-Fi 下的 AtomS3R 访问这台电脑。
    #
    # port=5000 的意思是：
    # 服务运行在 5000 端口。
    #
    # 运行后终端通常会显示：
    # Running on http://127.0.0.1:5000
    # Running on http://192.168.x.x:5000
    #
    # Arduino 代码里的 SERVER_URL 要使用 192.168.x.x 那个地址。
    app.run(host="0.0.0.0", port=5000)
