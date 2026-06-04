from pathlib import Path
import math


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "exports" / "screen_svgs"


def rgb565(value):
    r = (value >> 11) & 0x1F
    g = (value >> 5) & 0x3F
    b = value & 0x1F
    r = round(r * 255 / 31)
    g = round(g * 255 / 63)
    b = round(b * 255 / 31)
    return f"#{r:02x}{g:02x}{b:02x}"


COLORS = {
    "bg": "#0c100e",
    "plant": rgb565(0xAFE5),
    "dim_plant": "#37693a",
    "dark_green": rgb565(0x2589),
    "white": rgb565(0xFFFF),
    "sun": "#ffcd50",
    "active": "#6ebeff",
    "alert": "#ff4b41",
    "flower": "#ff96d2",
    "flower_pink": "#eeb0c4",
    "flower_center": "#f5dc96",
}


STATE_CONFIG = {
    "idle": {
        "title": "READY",
        "speech": ("READY TO", "EXPLORE"),
        "lux": 100,
        "eye": "center",
        "sunlit": False,
        "plant": "plant",
        "sound": "quiet",
    },
    "wilted": {
        "title": "WILTED",
        "speech": ("NEED REAL", "SUNLIGHT"),
        "lux": 10,
        "eye": "closed",
        "sunlit": False,
        "plant": "dim_plant",
        "sound": "quiet",
    },
    "need_sun": {
        "title": "NEED SUN",
        "speech": ("TAKE ME", "OUTSIDE"),
        "lux": 180,
        "eye": "center",
        "sunlit": False,
        "plant": "plant",
        "sound": "quiet",
    },
    "sunlight": {
        "title": "SUN",
        "speech": ("SUN FOUND", "FEEL ALIVE"),
        "lux": 1800,
        "eye": "up",
        "sunlit": True,
        "plant": "plant",
        "sound": "quiet",
    },
    "walking": {
        "title": "WALKING",
        "speech": ("WALK MODE", "LETS GO"),
        "lux": 650,
        "eye": "center",
        "sunlit": True,
        "plant": "plant",
        "sound": "quiet",
    },
}


def lux_to_lift(lux: float) -> float:
    if lux < 0:
        return 0.0
    dark_lux = 20.0
    bright_lux = 1800.0
    a = math.log10(lux + 1.0)
    amin = math.log10(dark_lux + 1.0)
    amax = math.log10(bright_lux + 1.0)
    v = (a - amin) / (amax - amin)
    return -10.0 + max(0.0, min(1.0, v)) * 32.0


def tag(name, attrs=None, body=""):
    attrs = attrs or {}
    attr_text = " ".join(f'{k}="{v}"' for k, v in attrs.items())
    if body:
        return f"<{name} {attr_text}>{body}</{name}>"
    return f"<{name} {attr_text}/>"


def circle(cx, cy, r, fill, extra=None):
    attrs = {"cx": cx, "cy": cy, "r": r, "fill": fill}
    if extra:
        attrs.update(extra)
    return tag("circle", attrs)


def round_rect(x, y, w, h, r, fill):
    return tag("rect", {"x": x, "y": y, "width": w, "height": h, "rx": r, "ry": r, "fill": fill})


def line(x1, y1, x2, y2, stroke, width=1):
    return tag("line", {
        "x1": x1,
        "y1": y1,
        "x2": x2,
        "y2": y2,
        "stroke": stroke,
        "stroke-width": width,
        "stroke-linecap": "round",
    })


def text(x, y, content, size, fill="#ffffff", anchor="middle", weight="700"):
    return tag("text", {
        "x": x,
        "y": y,
        "font-family": "monospace",
        "font-size": size,
        "font-weight": weight,
        "fill": fill,
        "text-anchor": anchor,
        "dominant-baseline": "middle",
    }, content)


def draw_pink_flower(cx, cy, base_r):
    petal_r = max(6, int(base_r * 50 / 100))
    spread = max(7, int(base_r * 52 / 100))
    center_r = max(3, int(base_r * 24 / 100))
    return "\n".join([
        circle(cx, cy - spread, petal_r, COLORS["flower_pink"]),
        circle(cx - spread, cy - 1, petal_r, COLORS["flower_pink"]),
        circle(cx + spread, cy - 1, petal_r, COLORS["flower_pink"]),
        circle(cx - spread // 2, cy + spread - 1, petal_r, COLORS["flower_pink"]),
        circle(cx + spread // 2, cy + spread - 1, petal_r, COLORS["flower_pink"]),
        circle(cx, cy, center_r, COLORS["flower_center"]),
    ])


def draw_eye(kind, cx, cy):
    dark = COLORS["dark_green"]
    if kind == "closed":
        return round_rect(cx - 6, cy - 2, 12, 4, 1, dark)
    if kind == "squeeze_left":
        return "\n".join([line(cx - 5, cy - 6, cx + 5, cy, dark, 2), line(cx + 5, cy, cx - 5, cy + 6, dark, 2)])
    if kind == "squeeze_right":
        return "\n".join([line(cx + 5, cy - 6, cx - 5, cy, dark, 2), line(cx - 5, cy, cx + 5, cy + 6, dark, 2)])
    offset_y = -3 if kind == "up" else 0
    return "\n".join([
        circle(cx, cy, 8, COLORS["white"]),
        circle(cx, cy + offset_y, 4, dark),
    ])


def s3r_svg(state_name, cfg):
    lift = round(lux_to_lift(cfg["lux"]))
    shake_x = 0
    y1 = 60 - lift
    y2 = 72 - (lift * 2) // 3
    y3 = 84 - lift // 3
    y4 = 96
    w1 = min(90, max(70, 80 + lift // 3))
    w2 = min(104, max(86, 96 + lift // 4))
    w3 = min(96, max(80, 88 + lift // 5))
    w4 = 72
    x1 = 64 - w1 // 2 + shake_x // 2
    x2 = 64 - w2 // 2 + shake_x // 3
    x3 = 64 - w3 // 2 + shake_x // 5
    x4 = 64 - w4 // 2
    plant_fill = COLORS[cfg["plant"]]
    left_flower_y = 50 - (lift * 3) // 4
    right_flower_y = left_flower_y
    mid_flower_y = 45 - lift
    eye_y = 80 - lift // 4

    parts = [
        tag("rect", {"x": 0, "y": 0, "width": 128, "height": 128, "fill": COLORS["bg"]}),
        round_rect(x1, y1, w1, 16, 4, plant_fill),
        round_rect(x2, y2, w2, 16, 4, plant_fill),
        round_rect(x3, y3, w3, 16, 4, plant_fill),
        round_rect(x4, y4, w4, 12, 4, plant_fill),
    ]

    if cfg["sunlit"]:
        parts.extend([
            draw_pink_flower(40, left_flower_y, 17),
            draw_pink_flower(88, right_flower_y, 17),
            draw_pink_flower(64, mid_flower_y, 19),
            circle(112, 18, 3, COLORS["sun"]),
            circle(105, 25, 1, COLORS["sun"]),
        ])
    else:
        parts.extend([
            circle(40, left_flower_y, 14, COLORS["white"]),
            circle(88, right_flower_y, 14, COLORS["white"]),
            circle(64, mid_flower_y, 16, COLORS["white"]),
        ])

    if cfg["sound"] == "quiet":
        parts.extend([circle(18, 42, 4, COLORS["flower"], {"fill": "none", "stroke": COLORS["flower"], "stroke-width": 1}), circle(18, 42, 2, COLORS["flower"])])

    eye_kind = cfg["eye"]
    parts.extend([draw_eye(eye_kind, 50, eye_y), draw_eye(eye_kind, 78, eye_y)])
    parts.append(text(64, 118, state_name.upper().replace("_", " "), 7, "#e6f0dc", weight="600"))

    return "\n".join([
        '<svg xmlns="http://www.w3.org/2000/svg" width="512" height="512" viewBox="0 0 128 128" shape-rendering="geometricPrecision">',
        f"<title>S3R {state_name}</title>",
        *parts,
        "</svg>",
    ])


def s3r_shaken_svg(state_name, cfg, bloom=False):
    lift = round(lux_to_lift(cfg["lux"]))
    shake_x = 7
    y1 = 60 - lift
    y2 = 72 - (lift * 2) // 3
    y3 = 84 - lift // 3
    y4 = 96
    w1 = min(90, max(70, 80 + lift // 3))
    w2 = min(104, max(86, 96 + lift // 4))
    w3 = min(96, max(80, 88 + lift // 5))
    w4 = 72
    x1 = 64 - w1 // 2 + shake_x // 2
    x2 = 64 - w2 // 2 + shake_x // 3
    x3 = 64 - w3 // 2 + shake_x // 5
    x4 = 64 - w4 // 2
    plant_fill = COLORS[cfg["plant"]]
    eye_y = 80 - lift // 4

    particles = [
        (27, 37, 17 if bloom else 14),
        (67, 59, 19 if bloom else 16),
        (101, 34, 17 if bloom else 14),
    ]

    parts = [
        tag("rect", {"x": 0, "y": 0, "width": 128, "height": 128, "fill": COLORS["bg"]}),
        round_rect(x1, y1, w1, 16, 4, plant_fill),
        round_rect(x2, y2, w2, 16, 4, plant_fill),
        round_rect(x3, y3, w3, 16, 4, plant_fill),
        round_rect(x4, y4, w4, 12, 4, plant_fill),
    ]

    for px, py, pr in particles:
        if bloom:
            parts.append(draw_pink_flower(px, py, pr))
        else:
            parts.append(circle(px, py, pr, COLORS["white"]))

    parts.extend([
        draw_eye("squeeze_left", 50 + shake_x // 3, eye_y),
        draw_eye("squeeze_right", 78 + shake_x // 3, eye_y),
        text(64, 118, "SHAKEN", 7, "#e6f0dc", weight="600"),
    ])

    return "\n".join([
        '<svg xmlns="http://www.w3.org/2000/svg" width="512" height="512" viewBox="0 0 128 128" shape-rendering="geometricPrecision">',
        f"<title>S3R {state_name}</title>",
        *parts,
        "</svg>",
    ])


def oled_svg(state_name, cfg):
    line0 = "SPROUT"
    line1 = cfg["title"]
    speech0, speech1 = cfg["speech"]
    parts = [
        tag("rect", {"x": 0, "y": 0, "width": 64, "height": 128, "fill": "#000000"}),
        tag("rect", {"x": 1, "y": 1, "width": 62, "height": 126, "rx": 6, "ry": 6, "fill": "none", "stroke": "#dff7ff", "stroke-width": 1}),
        tag("rect", {"x": 6, "y": 6, "width": 52, "height": 14, "rx": 4, "ry": 4, "fill": "#dff7ff"}),
        text(32, 13, line0, 7, "#000000", weight="700"),
        circle(10, 27, 1, "#dff7ff"),
        circle(16, 27, 1, "#dff7ff"),
        line(22, 27, 54, 27, "#dff7ff", 1),
        text(32, 37, line1, 8, "#dff7ff", weight="700"),
        tag("rect", {"x": 6, "y": 50, "width": 52, "height": 60, "rx": 5, "ry": 5, "fill": "none", "stroke": "#dff7ff", "stroke-width": 1}),
        text(32, 74, speech0, 6, "#dff7ff", weight="700"),
        text(32, 92, speech1, 6, "#dff7ff", weight="700"),
        line(10, 118, 54, 118, "#dff7ff", 1),
        circle(18, 122, 1, "#dff7ff"),
        circle(32, 122, 1, "#dff7ff"),
        circle(46, 122, 1, "#dff7ff"),
    ]
    return "\n".join([
        '<svg xmlns="http://www.w3.org/2000/svg" width="256" height="512" viewBox="0 0 64 128" shape-rendering="crispEdges">',
        f"<title>OLED {state_name}</title>",
        *parts,
        "</svg>",
    ])


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for state_name, cfg in STATE_CONFIG.items():
        (OUT_DIR / f"s3r_{state_name}.svg").write_text(s3r_svg(state_name, cfg), encoding="utf-8")
        (OUT_DIR / f"oled_{state_name}.svg").write_text(oled_svg(state_name, cfg), encoding="utf-8")
    (OUT_DIR / "s3r_shaken.svg").write_text(s3r_shaken_svg("shaken", STATE_CONFIG["need_sun"], bloom=False), encoding="utf-8")
    (OUT_DIR / "s3r_shaken_sunlight.svg").write_text(s3r_shaken_svg("shaken sunlight", STATE_CONFIG["sunlight"], bloom=True), encoding="utf-8")

    index = "\n".join(
        [
            "# Screen State SVG Exports",
            "",
            "Generated from the current Arduino drawing logic in `city_sprout_pahub_main_v4_no_flicker_canvas.ino`.",
            "",
            "S3R exports are stable representative frames for the five main plant states.",
            "OLED exports use the current fallback state text shown when no live speech text is available.",
            "",
            *[f"- `{name}`: `s3r_{name}.svg`, `oled_{name}.svg`" for name in STATE_CONFIG],
            "- `shaken`: `s3r_shaken.svg`",
            "- `shaken sunlight`: `s3r_shaken_sunlight.svg`",
            "",
        ]
    )
    (OUT_DIR / "README.md").write_text(index, encoding="utf-8")
    print(f"Exported {len(STATE_CONFIG) * 2 + 2} SVG files to {OUT_DIR}")


if __name__ == "__main__":
    main()
