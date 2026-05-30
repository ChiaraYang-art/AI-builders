"""GET / — 浏览器 TTS 调试页。"""

from __future__ import annotations

from flask import Blueprint

bp = Blueprint("voice_page", __name__)

VOICE_PAGE_HTML = """
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


@bp.route("/", methods=["GET"])
def index() -> str:
    return VOICE_PAGE_HTML
