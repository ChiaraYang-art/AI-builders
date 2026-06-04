<script setup>
import { computed } from "vue";

import { motionLabel } from "../api/sprout.js";
import { useAppNavigation, useSproutLiveContext } from "../composables/useAppNavigation.js";

const { latest, apiError, isLoading, refresh } = useSproutLiveContext();
const { go, randomInvite } = useAppNavigation();

const liveStatusText = computed(() => {
  if (apiError.value) {
    return `离线：${apiError.value}`;
  }

  if (isLoading.value && !latest.value) {
    return "连接 Flask 后端中...";
  }

  const tts = latest.value?.tts_status || "unknown";
  const lux = latest.value?.lux ?? "-";
  return `在线 · tts=${tts} · lux=${lux} · ${motionLabel(latest.value?.motion)}`;
});
</script>

<template>
  <aside class="demo-notes">
    <p>City Sprout</p>
    <h1>出走小芽APP Demo</h1>
    <span>网页端只显示小芽状态和文案；语音播放交给线下硬件小芽。</span>
    <p class="live-status" :class="{ error: apiError }">{{ liveStatusText }}</p>
    <div>
      <button @click="go('/home')">首页</button>
      <button @click="refresh">立即刷新</button>
      <button @click="randomInvite">随机散步</button>
    </div>
  </aside>
</template>
