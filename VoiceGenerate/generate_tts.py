import json
import os
import pathlib
import time

import dashscope
from dashscope.audio.tts_v2 import AudioFormat, SpeechSynthesizer


API_KEY = os.environ.get("DASHSCOPE_API_KEY")
if not API_KEY:
    raise RuntimeError("请先设置环境变量 DASHSCOPE_API_KEY")

dashscope.api_key = API_KEY

MODEL = "cosyvoice-v3-flash"
VOICE = "longhuhu_v3"

OUTPUT_DIR = pathlib.Path(__file__).resolve().parent / "tts_output"
OUTPUT_DIR.mkdir(exist_ok=True)

TEXTS = {
        "task_light_walk": [
        "一起出门晒晒太阳吧！",
        "出门找一段影子好不好？",
        "能不能带我去找一个腹肌最亮的小角落呀？",
        "一起出门吧！我想看看真正的天空。",
        "今天一起去收集一束金色阳光吧！",
    ],
    "task_sound_walk": [
        "我们去听听城市今天的声音吧。",
        "听听路上有没有小节奏。",
        "今天要不要出门去找小鸟唱的歌？",
        "出门收集三个让人开心的声音吧！",
        "一起去找一个温柔嗡嗡响的地方吧。",
    ],
    "task_color_walk": [
        "要不要一起出门去找五个绿色的东西呀？",
        "一起出门去发现一抹惊喜的粉色吧！",
        "要不要我们今天去找几种蓝色？",
        "今天天气真好，出门去找三种你觉得最像自己的颜色吧！",
        "一起找找今天是什么颜色！",
    ],
    "task_local_discovery": [
        "带出门我去找一个平时没注意的小细节吧！",
        "要不要去一个总是路过的角落看看？",
        "一起出门发现一种新的纹理好不好？",
        "出门找一个小小的好风景！",
        "附近开了一家新的花店，要一起去看看嘛？",
    ],
}


def synthesize_to_file(text, out_path):
    synthesizer = SpeechSynthesizer(
        model=MODEL,
        voice=VOICE,
        format=AudioFormat.MP3_24000HZ_MONO_256KBPS,
        volume=60,
        speech_rate=1.0,
        pitch_rate=1.0,
    )

    audio = synthesizer.call(text)
    if not audio:
        request_id = synthesizer.get_last_request_id()
        raise RuntimeError(f"语音合成没有返回音频，request_id={request_id}")

    out_path.write_bytes(audio)
    return synthesizer.get_last_request_id()


def main():
    manifest = []

    for category, lines in TEXTS.items():
        for index, text in enumerate(lines, start=1):
            filename = f"{category}_{index:02d}.mp3"
            out_path = OUTPUT_DIR / filename

            print(f"[生成] {filename} | {text}")

            if out_path.exists() and out_path.stat().st_size > 0:
                print(f"  已存在，跳过：{out_path}")
                manifest.append({
                    "category": category,
                    "index": index,
                    "text": text,
                    "voice": VOICE,
                    "model": MODEL,
                    "file": filename,
                    "skipped": True,
                })
                continue

            try:
                request_id = synthesize_to_file(text, out_path)
                manifest.append({
                    "category": category,
                    "index": index,
                    "text": text,
                    "voice": VOICE,
                    "model": MODEL,
                    "file": filename,
                    "request_id": request_id,
                })
                print(f"  完成：{out_path}")
                time.sleep(0.3)
            except Exception as e:
                print(f"  失败：{e}")

    manifest_path = OUTPUT_DIR / "manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )

    print(f"\n完成。输出目录：{OUTPUT_DIR}")
    print(f"清单文件：{manifest_path}")


if __name__ == "__main__":
    main()
