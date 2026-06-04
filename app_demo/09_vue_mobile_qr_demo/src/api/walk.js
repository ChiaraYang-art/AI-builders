const API_BASE = import.meta.env.VITE_API_BASE || "/api";

async function parseJsonResponse(response) {
  const data = await response.json().catch(() => ({}));

  if (!response.ok) {
    const message = data.error || `Request failed: ${response.status}`;
    throw new Error(message);
  }

  return data;
}

export async function startWalk(type) {
  const response = await fetch(`${API_BASE}/walk/start`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ type }),
  });

  return parseJsonResponse(response);
}

export async function uploadWalkPhoto(walkId, file) {
  const form = new FormData();
  form.append("walk_id", walkId);
  form.append("image", file, file.name || "photo.jpg");

  const response = await fetch(`${API_BASE}/walk/photo`, {
    method: "POST",
    body: form,
  });

  return parseJsonResponse(response);
}

export async function uploadWalkAudio(walkId, blob, filename = "recording.webm") {
  const form = new FormData();
  form.append("walk_id", walkId);
  form.append("audio", blob, filename);

  const response = await fetch(`${API_BASE}/walk/audio`, {
    method: "POST",
    body: form,
  });

  return parseJsonResponse(response);
}

export async function finishWalkDiary(walkId) {
  const response = await fetch(`${API_BASE}/walk/diary`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ walk_id: walkId }),
  });

  return parseJsonResponse(response);
}

export function buildWalkMediaUrl(path) {
  if (!path) {
    return null;
  }

  if (path.startsWith("http://") || path.startsWith("https://")) {
    return path;
  }

  return `${API_BASE}${path}`;
}
