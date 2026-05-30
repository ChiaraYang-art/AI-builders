"""传感器 payload 解析与类型转换。"""

from __future__ import annotations

from typing import Any

from ai.speech import SproutContext


def to_float(value: Any, default: float | None = 0.0) -> float | None:
    if value is None:
        return default
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def to_bool(value: Any) -> bool:
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        return value.strip().lower() in {"true", "1", "yes", "y"}
    return bool(value)


def parse_plant_payload(data: dict[str, Any]) -> tuple[SproutContext, dict[str, Any]]:
    state = str(data.get("state", "idle"))
    motion = str(data.get("motion", "still"))
    sound_state = str(data.get("sound_state", data.get("sound", "unknown")))
    place = str(data.get("place", "unknown"))

    lux = to_float(data.get("lux"), 0.0) or 0.0
    sound_level = to_float(data.get("sound_level"), 0.0) or 0.0
    sound_range = to_float(data.get("sound_range"), 0.0) or 0.0
    sound_variance = to_float(data.get("sound_variance"), 0.0) or 0.0

    env_ready = to_bool(data.get("env_ready", False))
    temperature_c = to_float(data.get("temperature_c", data.get("temperature")), None)
    humidity_percent = to_float(data.get("humidity_percent", data.get("humidity")), None)
    pressure_hpa = to_float(data.get("pressure_hpa", data.get("pressure")), None)
    gas_resistance_ohm = to_float(data.get("gas_resistance_ohm"), None)
    iaq = to_float(data.get("iaq"), None)
    iaq_accuracy = int(to_float(data.get("iaq_accuracy"), 0) or 0)

    ctx = SproutContext(
        state=state,
        lux=lux,
        motion=motion,
        sound_state=sound_state,
        place=place,
        env_ready=env_ready,
        temperature_c=temperature_c,
        humidity_percent=humidity_percent,
        iaq=iaq,
        sound_level=sound_level,
        sound_range=sound_range,
    )

    extras = {
        "device_id": data.get("device_id"),
        "sound_variance": sound_variance,
        "pressure_hpa": pressure_hpa,
        "gas_resistance_ohm": gas_resistance_ohm,
        "iaq_accuracy": iaq_accuracy,
    }
    return ctx, extras
