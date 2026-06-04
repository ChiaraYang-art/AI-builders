<script setup>
import { computed } from "vue";

import { motionLabel } from "../api/sprout.js";
import { useAppNavigation, useSproutLiveContext } from "../composables/useAppNavigation.js";

const { latest, apiError, isLoading, audioEnabled, refresh, enableAudio } =
  useSproutLiveContext();
const { go, randomInvite } = useAppNavigation();

const liveStatusText = computed(() => {
  if (apiError.value) {
    return `离线：${apiError.value}`;
  }

  if (isLoading.value && !latest.value) {
    return "连接 Flask 后端中…";
  }

  const tts = latest.value?.tts_status || "unknown";
  const lux = latest.value?.lux ?? "-";
  return `在线 · tts=${tts} · lux=${lux} · ${motionLabel(latest.value?.motion)}`;
});
</script>

<template>
  <aside class="demo-notes">
    <p>Vue Figma Strict</p>
    <h1>Figma 视觉优先版</h1>
    <span>P1：Vue Router + 首页/邀请/散步页接入 /latest 实时数据；根据小芽状态智能推荐散步类型。</span>
    <p class="live-status" :class="{ error: apiError }">{{ liveStatusText }}</p>
    <div>
      <button @click="go('/home')">首页</button>
      <button @click="refresh">立即刷新</button>
      <button v-if="!audioEnabled" @click="enableAudio">开启 TTS</button>
      <button @click="randomInvite">随机散步</button>
      <button @click="go('/diary')">日记</button>
      <button @click="go('/map')">地图</button>
    </div>
  </aside>
</template>
