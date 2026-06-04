import { onBeforeUnmount, onMounted, ref } from "vue";

import { buildAudioUrl, fetchLatest } from "../api/sprout.js";

export function useSproutLive(pollIntervalMs = 4000) {
  const latest = ref(null);
  const apiError = ref("");
  const isLoading = ref(false);
  const audioEnabled = ref(false);
  const lastPlayedSpeech = ref("");

  let pollTimer = null;
  let audioElement = null;

  async function refresh() {
    isLoading.value = true;

    try {
      latest.value = await fetchLatest();
      apiError.value = "";
      maybePlayAudio(latest.value);
    } catch (error) {
      apiError.value = error instanceof Error ? error.message : String(error);
    } finally {
      isLoading.value = false;
    }
  }

  function maybePlayAudio(data) {
    if (!audioEnabled.value || !data) {
      return;
    }

    if (data.tts_status !== "ready" || !data.audio_url || !data.speech) {
      return;
    }

    if (data.speech === lastPlayedSpeech.value) {
      return;
    }

    const url = buildAudioUrl(data);
    if (!url) {
      return;
    }

    if (!audioElement) {
      audioElement = new Audio();
    }

    if (!audioElement.paused && !audioElement.ended) {
      return;
    }

    audioElement.src = url;
    audioElement.play().catch(() => {});
    lastPlayedSpeech.value = data.speech;
  }

  function enableAudio() {
    audioEnabled.value = true;

    if (latest.value) {
      lastPlayedSpeech.value = "";
      maybePlayAudio(latest.value);
    }
  }

  onMounted(() => {
    refresh();
    pollTimer = window.setInterval(refresh, pollIntervalMs);
  });

  onBeforeUnmount(() => {
    if (pollTimer) {
      window.clearInterval(pollTimer);
      pollTimer = null;
    }

    if (audioElement) {
      audioElement.pause();
      audioElement = null;
    }
  });

  return {
    latest,
    apiError,
    isLoading,
    audioEnabled,
    refresh,
    enableAudio,
  };
}
