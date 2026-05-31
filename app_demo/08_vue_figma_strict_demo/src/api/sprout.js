const API_BASE = import.meta.env.VITE_API_BASE || "/api";

export async function fetchLatest() {
  const response = await fetch(`${API_BASE}/latest`);

  if (!response.ok) {
    throw new Error(`GET /latest failed: ${response.status}`);
  }

  return response.json();
}

export async function updateLlmEnabled(enabled) {
  const response = await fetch(`${API_BASE}/settings/llm`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ llm_enabled: enabled }),
  });

  if (!response.ok) {
    throw new Error(`POST /settings/llm failed: ${response.status}`);
  }

  return response.json();
}

export function buildAudioUrl(latest) {
  if (!latest?.audio_url) {
    return null;
  }

  const stamp = latest.tts_updated_at || latest.updated_at || String(Date.now());
  return `${API_BASE}${latest.audio_url}?t=${encodeURIComponent(stamp)}`;
}

export function stateTitle(state) {
  const titles = {
    idle: "有点安静",
    wilted: "有点蔫了",
    need_sun: "想晒太阳",
    sunlight: "充满阳光",
    walking: "正在散步",
  };

  return titles[state] || "小芽状态";
}

export function sproutAssetForState(state) {
  if (state === "sunlight" || state === "walking") {
    return "sprout-happy.svg";
  }

  if (state === "need_sun") {
    return "sprout-wilted.svg";
  }

  return "sprout-wilted.svg";
}

export function placeLabel(place) {
  const labels = {
    indoor: "在室内",
    outside: "在室外",
    unknown: "位置未知",
  };

  return labels[place] || "位置未知";
}

export function luxLabel(lux) {
  if (lux === null || lux === undefined || Number(lux) < 0) {
    return "光照未知";
  }

  if (lux < 50) {
    return "光照偏低";
  }

  if (lux < 300) {
    return "光照一般";
  }

  return "光照充足";
}

export function soundLabel(state) {
  const labels = {
    quiet: "很安静",
    active: "有环境声",
    intense: "声音较响",
    unknown: "声音未知",
  };

  return labels[state] || "声音未知";
}

export function motionLabel(motion) {
  const labels = {
    still: "静止",
    active: "在移动",
  };

  return labels[motion] || motion || "未知";
}

export function formatTempHumidity(temperatureC, humidityPercent) {
  const temp =
    temperatureC === null || temperatureC === undefined
      ? "--°C"
      : `${Number(temperatureC).toFixed(1)}°C`;
  const hum =
    humidityPercent === null || humidityPercent === undefined
      ? "--%"
      : `${Number(humidityPercent).toFixed(1)}%`;

  return `温度${temp} 湿度${hum}`;
}

export function formatUpdatedAt(updatedAt) {
  if (!updatedAt) {
    return "等待硬件上报…";
  }

  const date = new Date(updatedAt);
  if (Number.isNaN(date.getTime())) {
    return `最近更新：${updatedAt}`;
  }

  return `最近更新：${date.toLocaleString("zh-CN", { hour12: false })}`;
}

export function splitSpeechLines(text) {
  if (!text || !text.trim()) {
    return ["等待小芽说话……", "请确认 Flask 后端与硬件已连接"];
  }

  const cleaned = text.trim();
  const parts = cleaned
    .split(/(?<=[。！？…\n])/)
    .map((part) => part.trim())
    .filter(Boolean);

  if (parts.length >= 2) {
    return [parts[0], parts.slice(1).join("")];
  }

  if (cleaned.length <= 28) {
    return [cleaned, ""];
  }

  return [cleaned.slice(0, 28), cleaned.slice(28)];
}

export function buildSensorLines(latest) {
  if (!latest) {
    return ["等待数据…", "光照未知", "声音未知", "温度--°C 湿度--%"];
  }

  return [
    placeLabel(latest.place),
    luxLabel(latest.lux),
    soundLabel(latest.sound_state || latest.sound),
    formatTempHumidity(latest.temperature_c, latest.humidity_percent),
  ];
}

export function buildWalkSensorLines(latest) {
  if (!latest) {
    return ["在室外", "光照充足", "有城市声音", "温度--°C 湿度--%"];
  }

  const outdoorPlace = latest.place === "outside" ? "在室外" : "散步中";
  return [
    outdoorPlace,
    luxLabel(latest.lux),
    soundLabel(latest.sound_state || latest.sound),
    formatTempHumidity(latest.temperature_c, latest.humidity_percent),
  ];
}

export function suggestInviteType(latest) {
  if (!latest) {
    return "light";
  }

  if (
    latest.state === "wilted" ||
    latest.state === "need_sun" ||
    latest.state === "idle"
  ) {
    return "light";
  }

  if (latest.lux !== null && latest.lux !== undefined && latest.lux >= 300) {
    return "color";
  }

  return "light";
}
