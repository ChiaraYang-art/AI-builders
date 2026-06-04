"""POST /plant 散步流水账（speech events）接口测试。"""

from __future__ import annotations

import json
import os
import unittest

os.environ.setdefault("SPROUT_DEMO_BYPASS_WALK", "1")


class SpeechLedgerApiTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        from app import app

        cls.client = app.test_client()

    def _json(self, method: str, path: str, payload: dict | None = None) -> tuple[int, dict]:
        if method == "GET":
            resp = self.client.get(path)
        else:
            resp = self.client.post(
                path,
                data=json.dumps(payload or {}),
                content_type="application/json",
                headers={"Accept": "application/json"},
            )
        data = resp.get_json()
        assert data is not None, f"{method} {path} 未返回 JSON: {resp.data!r}"
        return resp.status_code, data

    def _plant(self, payload: dict) -> tuple[int, dict]:
        return self._json("POST", "/plant", payload)

    def _speech_events(self, events: list[dict]) -> list[dict]:
        return [event for event in events if event.get("kind") == "speech"]

    def test_no_active_walk_does_not_record_speech(self) -> None:
        status, latest = self._json("GET", "/latest")
        self.assertEqual(status, 200)
        self.assertIsNone(latest.get("active_walk"))

        status, plant = self._plant(
            {
                "state": "idle",
                "lux": 40.0,
                "motion": "still",
                "sound_state": "quiet",
            }
        )
        self.assertEqual(status, 200)
        self.assertTrue(plant.get("speech_full"))

        status, latest = self._json("GET", "/latest")
        self.assertEqual(status, 200)
        self.assertIsNone(latest.get("active_walk"))

    def test_plant_records_speech_during_active_walk(self) -> None:
        status, walk = self._json("POST", "/walk/start", {"type": "light"})
        self.assertEqual(status, 200)
        walk_id = walk["walk_id"]

        plant_payloads = [
            {
                "state": "walking",
                "lux": 520.0,
                "motion": "walking",
                "sound_state": "active",
                "place": "outside",
                "temperature_c": 25.0,
                "humidity_percent": 50.0,
            },
            {
                "state": "sunlight",
                "lux": 680.0,
                "motion": "walking",
                "sound_state": "quiet",
                "place": "outside",
                "temperature_c": 26.0,
                "humidity_percent": 48.0,
            },
        ]
        speeches: list[str] = []
        for payload in plant_payloads:
            status, plant = self._plant(payload)
            self.assertEqual(status, 200)
            speeches.append(plant["speech_full"])

        status, latest = self._json("GET", "/latest")
        self.assertEqual(status, 200)

        active_walk = latest.get("active_walk")
        self.assertIsNotNone(active_walk)
        assert active_walk is not None
        self.assertEqual(active_walk["walk_id"], walk_id)

        events = active_walk.get("events") or []
        self.assertTrue(any(event.get("kind") == "start" for event in events))

        speech_events = self._speech_events(events)
        self.assertGreaterEqual(len(speech_events), len(plant_payloads))

        for index, event in enumerate(speech_events[-len(plant_payloads) :]):
            self.assertEqual(event["kind"], "speech")
            self.assertTrue(event.get("time"))
            self.assertEqual(event["text"], speeches[index])
            sensor = event.get("sensor") or {}
            self.assertEqual(sensor.get("lux"), plant_payloads[index]["lux"])
            self.assertEqual(sensor.get("motion"), plant_payloads[index]["motion"])
            self.assertEqual(sensor.get("state"), plant_payloads[index]["state"])

    def test_diary_timeline_includes_speech_events(self) -> None:
        status, walk = self._json("POST", "/walk/start", {"type": "light"})
        self.assertEqual(status, 200)
        walk_id = walk["walk_id"]

        status, plant = self._plant(
            {
                "state": "sunlight",
                "lux": 600.0,
                "motion": "walking",
                "sound_state": "quiet",
                "place": "outside",
            }
        )
        self.assertEqual(status, 200)
        speech_text = plant["speech_full"]

        status, diary = self._json("POST", "/walk/diary", {"walk_id": walk_id})
        self.assertEqual(status, 200)

        timeline = diary.get("timeline") or []
        self.assertIsInstance(timeline, list)
        self.assertGreaterEqual(len(timeline), 2)
        joined = "\n".join(timeline)
        # LLM 生成 timeline 时可能略改措辞，但应保留 speech 流水账的核心内容
        core_phrase = speech_text.split("。")[1] if "。" in speech_text else speech_text
        self.assertTrue(
            speech_text in joined or core_phrase in joined,
            msg=f"timeline 未包含 speech 核心内容: {joined!r}",
        )


if __name__ == "__main__":
    unittest.main()
