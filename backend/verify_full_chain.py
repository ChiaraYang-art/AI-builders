"""
City Sprout 全链路自动化验证（对照 PRD 第 18、19 节）。

用法：
    cd backend
    python verify_full_chain.py
    python verify_full_chain.py --base-url http://127.0.0.1:5000
    python verify_full_chain.py --skip-tts-wait

环境变量：
    CITY_SPROUT_BASE_URL   默认 http://127.0.0.1:5000
    DASHSCOPE_API_KEY      可选；未配置时 TTS 步骤仅检查字段存在
"""

from __future__ import annotations

import argparse
import json
import os
import struct
import sys
import time
import zlib
from pathlib import Path
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

try:
    from dotenv import load_dotenv

    load_dotenv(Path(__file__).resolve().parent / ".env")
except ImportError:
    pass


class VerifyError(Exception):
    pass


class ChainVerifier:
    def __init__(
        self,
        base_url: str,
        skip_tts_wait: bool,
        tts_timeout: int,
        flask_client: Any | None = None,
    ) -> None:
        self.base_url = base_url.rstrip("/")
        self.skip_tts_wait = skip_tts_wait
        self.tts_timeout = tts_timeout
        self.flask_client = flask_client
        self.passed = 0
        self.failed = 0
        self.warnings = 0
        self.walk_id: str | None = None
        self.photo_url: str | None = None

    def ok(self, message: str) -> None:
        self.passed += 1
        print(f"[PASS] {message}")

    def warn(self, message: str) -> None:
        self.warnings += 1
        print(f"[WARN] {message}")

    def fail(self, message: str) -> None:
        self.failed += 1
        print(f"[FAIL] {message}", file=sys.stderr)

    def request(
        self,
        method: str,
        path: str,
        *,
        data: bytes | None = None,
        headers: dict[str, str] | None = None,
        content_type: str | None = None,
    ) -> tuple[int, bytes, str]:
        if self.flask_client is not None:
            kwargs: dict[str, Any] = {}
            if data is not None:
                kwargs["data"] = data
            if headers:
                kwargs["headers"] = headers
            if content_type:
                kwargs["content_type"] = content_type
            response = self.flask_client.open(path, method=method, **kwargs)
            body = response.get_data()
            ctype = response.content_type or ""
            return response.status_code, body, ctype

        url = f"{self.base_url}{path}"
        req_headers = dict(headers or {})
        if content_type:
            req_headers["Content-Type"] = content_type
        request = Request(url, data=data, headers=req_headers, method=method)
        try:
            with urlopen(request, timeout=60) as response:
                body = response.read()
                ctype = response.headers.get("Content-Type", "")
                return response.status, body, ctype
        except HTTPError as exc:
            return exc.code, exc.read(), exc.headers.get("Content-Type", "")
        except URLError as exc:
            raise VerifyError(f"无法连接 {url}: {exc}") from exc

    def request_json(
        self,
        method: str,
        path: str,
        *,
        payload: dict[str, Any] | None = None,
        headers: dict[str, str] | None = None,
    ) -> tuple[int, dict[str, Any] | None]:
        body = None
        req_headers = dict(headers or {})
        if payload is not None:
            body = json.dumps(payload).encode("utf-8")
            req_headers["Content-Type"] = "application/json"
        status, raw, _ = self.request(method, path, data=body, headers=req_headers)
        parsed = None
        if raw:
            try:
                parsed = json.loads(raw.decode("utf-8"))
            except json.JSONDecodeError:
                parsed = None
        return status, parsed

    def multipart(
        self,
        path: str,
        fields: dict[str, str],
        files: dict[str, tuple[str, bytes, str]],
    ) -> tuple[int, dict[str, Any] | None]:
        if self.flask_client is not None:
            from io import BytesIO

            form: dict[str, Any] = dict(fields)
            for name, (filename, content, _mime) in files.items():
                form[name] = (BytesIO(content), filename)
            response = self.flask_client.post(path, data=form, content_type="multipart/form-data")
            raw = response.get_data()
            parsed = json.loads(raw.decode("utf-8")) if raw else None
            return response.status_code, parsed

        boundary = "----CitySproutVerifyBoundary7MA4YWxk"
        chunks: list[bytes] = []

        for name, value in fields.items():
            chunks.append(f"--{boundary}\r\n".encode())
            chunks.append(
                f'Content-Disposition: form-data; name="{name}"\r\n\r\n{value}\r\n'.encode()
            )

        for name, (filename, content, mime) in files.items():
            chunks.append(f"--{boundary}\r\n".encode())
            chunks.append(
                (
                    f'Content-Disposition: form-data; name="{name}"; filename="{filename}"\r\n'
                    f"Content-Type: {mime}\r\n\r\n"
                ).encode()
            )
            chunks.append(content)
            chunks.append(b"\r\n")

        chunks.append(f"--{boundary}--\r\n".encode())
        body = b"".join(chunks)
        status, raw, _ = self.request(
            "POST",
            path,
            data=body,
            content_type=f"multipart/form-data; boundary={boundary}",
        )
        parsed = json.loads(raw.decode("utf-8")) if raw else None
        return status, parsed

    @staticmethod
    def tiny_png() -> bytes:
        width = 16
        height = 16

        def chunk(tag: bytes, data: bytes) -> bytes:
            crc = zlib.crc32(tag + data) & 0xFFFFFFFF
            return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)

        ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
        raw_rows = b"".join(b"\x00" + (b"\x00\xff\x00" * width) for _ in range(height))
        png = b"\x89PNG\r\n\x1a\n"
        png += chunk(b"IHDR", ihdr)
        png += chunk(b"IDAT", zlib.compress(raw_rows))
        png += chunk(b"IEND", b"")
        return png

    @staticmethod
    def tiny_wav() -> bytes:
        sample_rate = 8000
        seconds = 1
        num_samples = sample_rate * seconds
        data = b"".join(
            struct.pack("<h", int(8000 * 0.15 * (1 if i % 40 < 20 else -1)))
            for i in range(num_samples)
        )
        data_size = len(data)
        byte_rate = sample_rate * 2
        header = struct.pack(
            "<4sI4s4sIHHIIHH4sI",
            b"RIFF",
            36 + data_size,
            b"WAVE",
            b"fmt ",
            16,
            1,
            1,
            sample_rate,
            byte_rate,
            2,
            16,
            b"data",
            data_size,
        )
        return header + data

    def assert_true(self, condition: bool, message: str) -> None:
        if condition:
            self.ok(message)
        else:
            self.fail(message)
            raise VerifyError(message)

    def step_server_reachable(self) -> None:
        status, body, _ = self.request("GET", "/latest")
        self.assert_true(status == 200, "P0: GET /latest 服务可达")
        latest = json.loads(body.decode("utf-8"))
        for key in ("state", "speech", "lux", "motion", "updated_at"):
            self.assert_true(key in latest, f"P0: /latest 含字段 {key}")

    def step_plant_json(self) -> dict[str, Any]:
        payload = {
            "state": "need_sun",
            "lux": 32.5,
            "motion": "still",
            "sound_state": "quiet",
            "sound_level": 0.08,
            "sound_range": 0.04,
            "place": "indoor",
            "env_ready": True,
            "temperature_c": 24.5,
            "humidity_percent": 58.0,
        }
        status, data = self.request_json(
            "POST",
            "/plant",
            payload=payload,
            headers={"Accept": "application/json"},
        )
        self.assert_true(status == 200 and data is not None, "P0: POST /plant (JSON)")
        assert data is not None
        for key in ("speech", "speech_short", "speech_full", "state"):
            self.assert_true(bool(data.get(key)), f"P0: /plant JSON 返回 {key}")
        self.assert_true(data["state"] == "need_sun", "P0: /plant 保留 state=need_sun")
        return data

    def step_plant_text(self) -> str:
        payload = {
            "state": "sunlight",
            "lux": 580.0,
            "motion": "walking",
            "sound_state": "active",
            "sound_level": 0.22,
            "sound_range": 0.15,
            "place": "outside",
            "env_ready": True,
            "temperature_c": 26.0,
            "humidity_percent": 45.0,
        }
        status, body, ctype = self.request(
            "POST",
            "/plant",
            data=json.dumps(payload).encode("utf-8"),
            content_type="application/json",
        )
        text = body.decode("utf-8").strip()
        self.assert_true(status == 200, "P0: POST /plant (Arduino text/plain)")
        self.assert_true("text/plain" in ctype, "P0: Arduino 响应为 text/plain")
        self.assert_true(len(text) > 0, "P0: Arduino 收到非空 speech_short")
        return text

    def step_latest_sync(self, speech_short: str) -> dict[str, Any]:
        status, body, _ = self.request("GET", "/latest")
        latest = json.loads(body.decode("utf-8"))
        self.assert_true(status == 200, "P0: GET /latest 二次读取")
        self.assert_true(latest.get("speech_short") == speech_short, "P0: /latest 与 Arduino 短句同步")
        self.assert_true(latest.get("speech") == latest.get("speech_full"), "P0: speech 与 speech_full 一致")
        for key in ("tts_status", "active_walk", "walk_diary", "atlas_unlocked"):
            if key in latest:
                self.ok(f"P1/P2: /latest 含扩展字段 {key}")
            else:
                self.fail(
                    f"P1/P2: /latest 缺少 {key}；请重启 sprout_server.py 到最新版后再验证"
                )
                raise VerifyError(f"missing latest field: {key}")
        return latest

    def step_tts_chain(self, latest: dict[str, Any]) -> None:
        if not os.environ.get("DASHSCOPE_API_KEY"):
            self.warn("未配置 DASHSCOPE_API_KEY，跳过 TTS ready / MP3 强校验")
            self.assert_true("tts_status" in latest, "P1: /latest 暴露 tts_status 字段")
            return

        if self.skip_tts_wait:
            self.warn("已 --skip-tts-wait，跳过 TTS 等待")
            return

        deadline = time.time() + self.tts_timeout
        ready = latest.get("tts_status") == "ready"
        while not ready and time.time() < deadline:
            time.sleep(1.5)
            status, body, _ = self.request("GET", "/latest")
            latest = json.loads(body.decode("utf-8"))
            ready = latest.get("tts_status") == "ready"

        self.assert_true(ready, f"P1: TTS 在 {self.tts_timeout}s 内变为 ready")
        status, body, ctype = self.request("GET", "/audio/latest.mp3")
        self.assert_true(status == 200, "P1: GET /audio/latest.mp3 返回 200")
        self.assert_true(len(body) > 256, "P1: latest.mp3 非空")
        self.assert_true("audio" in ctype, "P1: MP3 Content-Type 正确")

    def step_walk_color(self) -> None:
        status, data = self.request_json("POST", "/walk/start", payload={"type": "color"})
        self.assert_true(status == 200 and data is not None, "P0/P1: POST /walk/start (color)")
        assert data is not None
        self.walk_id = data.get("walk_id")
        self.assert_true(bool(self.walk_id), "P0/P1: 返回 walk_id")

        status, photo = self.multipart(
            "/walk/photo",
            {"walk_id": self.walk_id},
            {"image": ("verify_green.png", self.tiny_png(), "image/png")},
        )
        self.assert_true(status == 200 and photo is not None, "P0: POST /walk/photo")
        assert photo is not None
        for key in ("objects", "colors", "summary", "progress", "url"):
            self.assert_true(key in photo, f"P0: /walk/photo 返回 {key}")
        progress = photo.get("progress") or {}
        self.assert_true(progress.get("current", 0) >= 1, "P0: Color Walk 进度 current>=1")
        self.photo_url = photo.get("url")

    def step_walk_sound(self) -> None:
        status, data = self.request_json("POST", "/walk/start", payload={"type": "sound"})
        self.assert_true(status == 200 and data is not None, "P1: POST /walk/start (sound)")
        assert data is not None
        walk_id = data.get("walk_id")
        self.assert_true(bool(walk_id), "P1: Sound Walk 返回 walk_id")

        status, audio = self.multipart(
            "/walk/audio",
            {"walk_id": walk_id},
            {"audio": ("verify.wav", self.tiny_wav(), "audio/wav")},
        )
        self.assert_true(status == 200 and audio is not None, "P1: POST /walk/audio")
        assert audio is not None
        for key in ("sounds", "summary", "active_walk"):
            self.assert_true(key in audio, f"P1: /walk/audio 返回 {key}")
        self.walk_id = walk_id

    def step_walk_diary(self) -> dict[str, Any]:
        assert self.walk_id
        status, diary = self.request_json(
            "POST",
            "/walk/diary",
            payload={"walk_id": self.walk_id},
        )
        self.assert_true(status == 200 and diary is not None, "P1: POST /walk/diary")
        assert diary is not None
        for key in ("title", "timeline", "essay", "map_summary"):
            self.assert_true(key in diary, f"P1: /walk/diary 返回 {key}")
        self.assert_true(isinstance(diary.get("timeline"), list), "P1: timeline 为数组")
        self.assert_true(len(diary.get("timeline", [])) >= 1, "P1: timeline 非空")
        return diary

    def step_latest_after_walk(self, diary: dict[str, Any]) -> None:
        status, body, _ = self.request("GET", "/latest")
        latest = json.loads(body.decode("utf-8"))
        self.assert_true(status == 200, "P1: 散步完成后 GET /latest")
        stored = latest.get("walk_diary") or {}
        self.assert_true(stored.get("title") == diary.get("title"), "P1: /latest.walk_diary 已写入")
        atlas = latest.get("atlas_unlocked") or []
        self.assert_true(isinstance(atlas, list) and len(atlas) >= 1, "P1: /latest.atlas_unlocked 非空")
        self.assert_true(
            "Sound Sprout" in atlas or "Night Sprout" in atlas,
            "P1: Sound Walk 解锁图鉴项",
        )

    def step_walk_media(self) -> None:
        if not self.walk_id or not self.photo_url:
            self.warn("无 photo_url，跳过 /walk/media 校验")
            return
        status, body, ctype = self.request("GET", self.photo_url)
        self.assert_true(status == 200, "P2: GET /walk/media 照片可访问")
        self.assert_true(len(body) > 0, "P2: 照片内容非空")

    def step_walk_types(self) -> None:
        for walk_type in ("light", "local"):
            status, data = self.request_json("POST", "/walk/start", payload={"type": walk_type})
            self.assert_true(status == 200 and data is not None, f"P1: POST /walk/start ({walk_type})")

    def run(self) -> int:
        print("=" * 60)
        print("City Sprout 全链路验证")
        mode = "in-process (Flask test_client)" if self.flask_client is not None else "HTTP"
        print(f"模式     = {mode}")
        print(f"BASE_URL = {self.base_url}")
        print(f"DASHSCOPE_API_KEY = {'已配置' if os.environ.get('DASHSCOPE_API_KEY') else '未配置'}")
        print("=" * 60)

        try:
            self.step_server_reachable()
            self.step_plant_json()
            speech_short = self.step_plant_text()
            latest = self.step_latest_sync(speech_short)
            self.step_tts_chain(latest)
            self.step_walk_color()
            self.step_walk_media()
            self.step_walk_sound()
            diary = self.step_walk_diary()
            self.step_latest_after_walk(diary)
            self.step_walk_types()
        except VerifyError:
            pass

        print("-" * 60)
        print(f"通过: {self.passed}  失败: {self.failed}  警告: {self.warnings}")
        if self.failed:
            print("结果: 未通过")
            return 1
        print("结果: 全部自动化项通过")
        return 0


def main() -> None:
    parser = argparse.ArgumentParser(description="City Sprout 全链路 API 验证")
    parser.add_argument(
        "--base-url",
        default=os.environ.get("CITY_SPROUT_BASE_URL", "http://127.0.0.1:5000"),
    )
    parser.add_argument("--skip-tts-wait", action="store_true")
    parser.add_argument("--tts-timeout", type=int, default=45)
    parser.add_argument(
        "--in-process",
        action="store_true",
        help="使用 Flask test_client，无需单独启动 sprout_server.py",
    )
    args = parser.parse_args()

    flask_client = None
    if args.in_process:
        from sprout_server import app

        flask_client = app.test_client()

    verifier = ChainVerifier(
        args.base_url,
        args.skip_tts_wait,
        args.tts_timeout,
        flask_client=flask_client,
    )
    raise SystemExit(verifier.run())


if __name__ == "__main__":
    main()
