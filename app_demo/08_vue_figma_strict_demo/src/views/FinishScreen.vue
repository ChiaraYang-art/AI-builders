<script setup>
import { computed } from "vue";

import { buildWalkMediaUrl } from "../api/walk.js";
import FigmaBottomNav from "../components/FigmaBottomNav.vue";
import { useAppNavigation, useSproutLiveContext } from "../composables/useAppNavigation.js";
import { asset } from "../utils/assets.js";

const props = defineProps({
  variant: {
    type: String,
    default: "",
  },
});

const { latest } = useSproutLiveContext();
const { go, randomInvite } = useAppNavigation();

const diary = computed(() => latest.value?.walk_diary);
const isColor = computed(() => props.variant === "color" || diary.value?.type === "color");
const finishText = computed(() => {
  if (diary.value?.essay) {
    return diary.value.essay;
  }

  if (latest.value?.speech_full) {
    return latest.value.speech_full;
  }

  return isColor.value
    ? "我把今天路上的绿色都记住了。谢谢你带我去看真实的颜色，我觉得自己又长出了一点新叶。"
    : "刚才的光比房间亮很多，我还听见了风和脚步声。谢谢你带我出去，我觉得自己又长出了一点新叶。";
});
const heroPhoto = computed(() => {
  const first = diary.value?.photos?.[0];
  return first ? buildWalkMediaUrl(first.url) : null;
});
</script>

<template>
  <section class="screen">
    <p class="time">9:41</p>
    <button type="button" class="back" @click="go('/home')" aria-label="返回">‹</button>
    <h1 class="title with-back">散步完成</h1>
    <div class="finish-hero">
      <img v-if="heroPhoto" :src="heroPhoto" alt="散步照片" class="finish-photo" />
      <img v-else :src="asset('sprout-happy.svg')" alt="开心的小芽" />
    </div>
    <h2 class="finish-title">{{ diary?.title || "我活过来了" }}</h2>
    <article class="finish-text">{{ finishText }}</article>
    <div class="finish-actions">
      <button type="button" class="primary" @click="go('/home')">返回首页</button>
      <button
        type="button"
        class="secondary"
        @click="go(isColor ? '/diary/log/color' : '/diary/log')"
      >
        查看散步日记
      </button>
    </div>
    <article class="saved-card">
      <strong>已保存</strong>
      <span>这次散步会出现在小芽日记里。</span>
    </article>
    <FigmaBottomNav active="diary" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
