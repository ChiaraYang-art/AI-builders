"""
本地测试 POST /plant 与 GET /latest AI 对话接口。

用法：
    cd backend
    export DASHSCOPE_API_KEY="your-key"   # 可选；未设置则走规则兜底
    python test_dialogue_api.py
"""

from __future__ import annotations

import json
import os
import sys

from sprout_server import app


PLANT_PAYLOAD = {
    "device_id": "sprout_test_001",
    "state": "need_sun",
    "lux": 32.5,
    "motion": "still",
    "sound_level": 0.12,
    "sound_range": 0.08,
    "sound_state": "stable",
    "place": "indoor",
    "temperature": 24.5,
    "humidity": 58.0,
    "pressure": 1012.0,
    "iaq": 45,
}


def run_tests() -> None:
    has_key = bool(os.environ.get("DASHSCOPE_API_KEY"))
    print(f"DASHSCOPE_API_KEY: {'已配置' if has_key else '未配置（将使用规则兜底）'}")
    print("-" * 60)

    client = app.test_client()

    # 1. POST /plant JSON 响应（APP）
    resp = client.post(
        "/plant",
        json=PLANT_PAYLOAD,
        headers={"Accept": "application/json"},
    )
    assert resp.status_code == 200, resp.data
    plant_json = resp.get_json()
    assert plant_json is not None
    assert "speech_short" in plant_json
    assert "speech_full" in plant_json
    assert plant_json["state"] == "need_sun"
    print("[OK] POST /plant (JSON)")
    print(json.dumps(plant_json, ensure_ascii=False, indent=2))

    # 2. POST /plant 纯文本响应（Arduino 兼容）
    resp_text = client.post("/plant", json=PLANT_PAYLOAD)
    assert resp_text.status_code == 200
    assert resp_text.content_type.startswith("text/plain")
    text_short = resp_text.data.decode("utf-8")
    assert text_short.strip(), "speech_short 不应为空"
    print("[OK] POST /plant (text/plain 兼容 Arduino)")
    print("  speech_short:", text_short)

    # 3. GET /latest（应与最近一次 POST 结果一致）
    resp_latest = client.get("/latest")
    assert resp_latest.status_code == 200
    latest = resp_latest.get_json()
    assert latest["speech"] == latest["speech_full"]
    assert latest["speech_short"] == text_short
    assert latest["speech_full"] == latest["speech"]
    assert latest["state"] == "need_sun"
    assert "updated_at" in latest
    print("[OK] GET /latest")
    print(json.dumps(
        {k: latest[k] for k in ("state", "speech", "speech_short", "lux", "motion", "sound", "updated_at")},
        ensure_ascii=False,
        indent=2,
    ))

    print("-" * 60)
    print("全部测试通过。")


if __name__ == "__main__":
    try:
        run_tests()
    except AssertionError as exc:
        print(f"[FAIL] {exc}", file=sys.stderr)
        sys.exit(1)
