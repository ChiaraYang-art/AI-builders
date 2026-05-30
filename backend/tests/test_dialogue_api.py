"""
测试 POST /plant 与 GET /latest AI 对话接口（目标：app.py）。

从 deploy/.env 读取配置（API Key、服务器地址等），支持本地与远程两种模式。

用法：
    cd backend
    python -m tests.test_dialogue_api

    # 或
    python tests/test_dialogue_api.py

环境变量（写在 deploy/.env）：
    SPROUT_SERVER_URL   服务器根地址，如 http://111.229.81.45:5000
    TEST_MODE           local（默认，Flask 内存测试）或 remote（HTTP 请求配置的服务器）
    DASHSCOPE_API_KEY   LLM 密钥（local 模式用于 qwen-plus；remote 模式仅作展示）
"""

from __future__ import annotations

import json
import os
import sys
import urllib.error
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from dotenv import load_dotenv

BACKEND_DIR = Path(__file__).resolve().parent.parent
REPO_ROOT = BACKEND_DIR.parent
ENV_PATH = REPO_ROOT / "deploy" / ".env"

if str(BACKEND_DIR) not in sys.path:
    sys.path.insert(0, str(BACKEND_DIR))

PLANT_PAYLOAD: dict[str, Any] = {
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


@dataclass
class TestConfig:
    server_url: str
    test_mode: str
    has_api_key: bool
    llm_model: str
    env_path: Path


def load_config() -> TestConfig:
    if ENV_PATH.exists():
        load_dotenv(ENV_PATH)
    else:
        load_dotenv(REPO_ROOT / "deploy" / ".env.example")

    port = os.environ.get("SPROUT_PORT", "5000")
    server_url = os.environ.get("SPROUT_SERVER_URL", f"http://127.0.0.1:{port}").rstrip("/")
    test_mode = os.environ.get("TEST_MODE", "local").strip().lower()

    return TestConfig(
        server_url=server_url,
        test_mode=test_mode,
        has_api_key=bool(os.environ.get("DASHSCOPE_API_KEY")),
        llm_model=os.environ.get("DASHSCOPE_LLM_MODEL", "qwen-plus"),
        env_path=ENV_PATH,
    )


def _http_request(
    method: str,
    url: str,
    *,
    json_body: dict[str, Any] | None = None,
    accept: str | None = None,
    timeout: float = 30.0,
) -> tuple[int, str, str]:
    headers = {"Content-Type": "application/json"}
    if accept:
        headers["Accept"] = accept

    data = None
    if json_body is not None:
        data = json.dumps(json_body).encode("utf-8")

    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        body = resp.read().decode("utf-8")
        content_type = resp.headers.get("Content-Type", "")
        return resp.status, body, content_type


def _assert_plant_json(plant_json: dict[str, Any]) -> None:
    assert "speech_short" in plant_json, "响应缺少 speech_short，可能未部署新版本"
    assert "speech_full" in plant_json, "响应缺少 speech_full，可能未部署新版本"
    assert plant_json["state"] == "need_sun"
    assert plant_json["speech_short"].strip(), "speech_short 不应为空"
    assert plant_json["speech_full"].strip(), "speech_full 不应为空"


def _assert_latest(latest: dict[str, Any], text_short: str) -> None:
    assert "speech_short" in latest, "latest 缺少 speech_short，可能未部署新版本"
    assert latest["speech"] == latest["speech_full"]
    assert latest["speech_short"] == text_short
    assert latest["speech_full"] == latest["speech"]
    assert latest["state"] == "need_sun"
    assert latest.get("updated_at"), "updated_at 不应为空"


def _print_summary(plant_json: dict[str, Any], latest: dict[str, Any]) -> None:
    print("[OK] POST /plant (JSON)")
    print(json.dumps(plant_json, ensure_ascii=False, indent=2))
    print("[OK] POST /plant (text/plain 兼容 Arduino)")
    print("  speech_short:", latest["speech_short"])
    print("[OK] GET /latest")
    print(
        json.dumps(
            {
                k: latest[k]
                for k in (
                    "state",
                    "speech",
                    "speech_short",
                    "lux",
                    "motion",
                    "sound",
                    "tts_status",
                    "updated_at",
                )
                if k in latest
            },
            ensure_ascii=False,
            indent=2,
        )
    )


def run_local_tests() -> None:
    from app import app  # backend/app.py

    client = app.test_client()

    resp = client.post(
        "/plant",
        json=PLANT_PAYLOAD,
        headers={"Accept": "application/json"},
    )
    assert resp.status_code == 200, resp.data
    plant_json = resp.get_json()
    assert plant_json is not None
    _assert_plant_json(plant_json)

    resp_text = client.post("/plant", json=PLANT_PAYLOAD)
    assert resp_text.status_code == 200
    assert resp_text.content_type.startswith("text/plain")
    text_short = resp_text.data.decode("utf-8")
    assert text_short.strip(), "speech_short 不应为空"

    resp_latest = client.get("/latest")
    assert resp_latest.status_code == 200
    latest = resp_latest.get_json()
    _assert_latest(latest, text_short)
    _print_summary(plant_json, latest)


def run_remote_tests(config: TestConfig) -> None:
    plant_url = f"{config.server_url}/plant"
    latest_url = f"{config.server_url}/latest"

    status, body, content_type = _http_request(
        "POST",
        plant_url,
        json_body=PLANT_PAYLOAD,
        accept="application/json",
    )
    assert status == 200, f"POST /plant 失败: HTTP {status}, body={body}"
    assert "application/json" in content_type, (
        f"POST /plant 未返回 JSON（当前可能是旧版服务）: {body[:200]}"
    )
    plant_json = json.loads(body)
    _assert_plant_json(plant_json)

    status, body, content_type = _http_request("POST", plant_url, json_body=PLANT_PAYLOAD)
    assert status == 200, f"POST /plant text 失败: HTTP {status}"
    assert "text/plain" in content_type, f"期望 text/plain，实际: {content_type}"
    text_short = body.strip()
    assert text_short, "speech_short 不应为空"

    status, body, _ = _http_request("GET", latest_url)
    assert status == 200, f"GET /latest 失败: HTTP {status}"
    latest = json.loads(body)
    _assert_latest(latest, text_short)
    _print_summary(plant_json, latest)


def run_tests() -> None:
    config = load_config()

    print(f"配置文件: {config.env_path}{'' if config.env_path.exists() else '（不存在，已回退 .env.example）'}")
    print(f"测试模式: {config.test_mode}")
    print(f"服务器:   {config.server_url}")
    print(f"LLM 模型: {config.llm_model}")
    print(f"DASHSCOPE_API_KEY: {'已配置' if config.has_api_key else '未配置（local 模式将使用规则兜底）'}")
    print("-" * 60)

    try:
        if config.test_mode == "remote":
            run_remote_tests(config)
        else:
            run_local_tests()
    except urllib.error.URLError as exc:
        print(f"[FAIL] 无法连接服务器 {config.server_url}: {exc}", file=sys.stderr)
        sys.exit(1)

    print("-" * 60)
    print("全部测试通过。")


if __name__ == "__main__":
    try:
        run_tests()
    except AssertionError as exc:
        print(f"[FAIL] {exc}", file=sys.stderr)
        sys.exit(1)
