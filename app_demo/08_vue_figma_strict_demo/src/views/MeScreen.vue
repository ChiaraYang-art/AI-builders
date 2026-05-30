<script setup>
import { computed } from "vue";

import { motionLabel, stateTitle } from "../api/sprout.js";
import FigmaBottomNav from "../components/FigmaBottomNav.vue";
import { useAppNavigation, useSproutLiveContext } from "../composables/useAppNavigation.js";
import { asset } from "../utils/assets.js";

const { latest, apiError } = useSproutLiveContext();
const { go, randomInvite } = useAppNavigation();

const profileSubtitle = computed(() => {
  if (apiError.value) {
    return "设备离线 · 请检查 Flask 后端";
  }

  return `成长 18 天 · 今日${stateTitle(latest.value?.state || "idle")}`;
});

const deviceStatus = computed(() => {
  if (apiError.value) {
    return "未连接";
  }

  const lux = latest.value?.lux ?? "-";
  return `已连接 · lux ${lux} · ${motionLabel(latest.value?.motion)}`;
});
</script>

<template>
  <section class="screen">
    <p class="time">9:41</p>
    <h1 class="title">我的</h1>
    <article class="profile-card">
      <img :src="asset('sprout-happy.svg')" alt="我的小芽" />
      <div>
        <h2>Chiara的小芽</h2>
        <p>{{ profileSubtitle }}</p>
        <p>已一起散步7次</p>
        <p>成长值：65</p>
        <span class="growth"><i></i></span>
      </div>
    </article>
    <article class="settings-card">
      <button>设备状态<span>{{ deviceStatus }} ›</span></button>
      <button>语音音色<span>longanyang ›</span></button>
      <button>提醒时间<span>›</span></button>
      <button>数据权限<span>›</span></button>
    </article>
    <button class="atlas-entry" @click="go('/atlas')">
      <span>我解锁的小芽</span>
      <i>›</i>
      <strong></strong><strong></strong><strong></strong><strong></strong>
    </button>
    <FigmaBottomNav active="me" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
