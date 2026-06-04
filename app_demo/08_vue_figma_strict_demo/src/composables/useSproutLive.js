import { onBeforeUnmount, onMounted, ref } from "vue";

import { fetchLatest } from "../api/sprout.js";

export function useSproutLive(pollIntervalMs = 4000) {
  const latest = ref(null);
  const apiError = ref("");
  const isLoading = ref(false);
  const audioEnabled = ref(false);

  let pollTimer = null;

  async function refresh() {
    isLoading.value = true;

    try {
      latest.value = await fetchLatest();
      apiError.value = "";
    } catch (error) {
      apiError.value = error instanceof Error ? error.message : String(error);
    } finally {
      isLoading.value = false;
    }
  }

  function enableAudio() {
    // Web audio is intentionally disabled for the app demo.
    // The physical sprout hardware is responsible for speaking.
    audioEnabled.value = false;
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
