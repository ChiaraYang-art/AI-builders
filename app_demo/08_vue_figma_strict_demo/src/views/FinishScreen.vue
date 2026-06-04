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

const rawDiary = computed(() => latest.value?.walk_diary);
const isColor = computed(() => props.variant === "color");
const diary = computed(() => {
  const current = rawDiary.value;

  if (!current) {
    return null;
  }

  if (isColor.value) {
    return current.type === "color" ? current : null;
  }

  return current.type === "color" ? null : current;
});

const finishTitle = computed(() => diary.value?.title || "我活过来了！");
const finishText = computed(() => {
  if (diary.value?.essay) {
    return diary.value.essay;
  }

  return isColor.value
    ? "我把今天路上的绿色都记住了。谢谢你带我去看真实的颜色，我觉得自己又长出了一点新叶。"
    : "刚才的光比房间亮很多，我还听见了风和脚步声。谢谢你带我出去，我觉得自己又长出了一点新叶。";
});
const photoItems = computed(() => (isColor.value ? diary.value?.photos || [] : []));
</script>

<template>
  <section class="screen finish-screen" :class="{ 'finish-screen-color': isColor }">
    <div class="finish-scroll">
      <p class="time">9:41</p>
      <button type="button" class="back" @click="go('/home')" aria-label="返回">‹</button>
      <h1 class="title with-back">散步完成</h1>
      <div class="finish-hero">
        <img :src="asset('sprout_sunlight 2.svg')" alt="开心的小芽" />
        <h2 class="finish-title">{{ finishTitle }}</h2>
      </div>
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
      <div v-if="isColor && photoItems.length" class="finish-photo-gallery">
        <img
          v-for="(photo, index) in photoItems"
          :key="photo.filename || index"
          :src="buildWalkMediaUrl(photo.url)"
          alt="Color Walk 收集到的绿色照片"
        />
      </div>
      <article class="saved-card">
        <strong>已保存</strong>
        <span>这次散步会出现在小芽日记里。</span>
      </article>
    </div>
    <FigmaBottomNav active="diary" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
