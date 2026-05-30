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
const essayBody = computed(() => {
  if (diary.value?.essay) {
    return diary.value.essay;
  }

  if (latest.value?.speech_full) {
    return latest.value.speech_full;
  }

  return "今天我们走到了有树影的路边。阳光先落在我的叶子上，然后慢慢变暖。";
});
const photoItems = computed(() => diary.value?.photos || []);
</script>

<template>
  <section class="screen scroll-screen">
    <p class="time">9:41</p>
    <button type="button" class="back" @click="go('/diary')" aria-label="返回">‹</button>
    <h1 class="title with-back">小芽日记</h1>
    <p class="subtitle with-back">{{ isColor ? "Color Walk 小作文" : "今日散步小作文" }}</p>
    <div class="tabs">
      <button @click="go(isColor ? '/diary/log/color' : '/diary/log')">流水账</button>
      <button class="active">小作文</button>
    </div>
    <article class="diary-card essay">
      <h2>{{ diary?.title || (isColor ? "今天我收集了很多绿色" : "今天我听见了春天") }}</h2>
      <p>{{ essayBody }}</p>
      <div v-if="photoItems.length" class="color-photo-grid diary-photo-grid essay-photos">
        <img
          v-for="(photo, index) in photoItems"
          :key="photo.filename || index"
          :src="buildWalkMediaUrl(photo.url)"
          alt="散步照片"
        />
      </div>
      <img
        v-else-if="isColor"
        class="diary-color-photos essay-photos"
        :src="asset('diary-color-walk-photos.svg')"
        alt="Color Walk 照片记录"
      />
      <p v-if="isColor && !photoItems.length">
        这些绿色不是一种颜色。它们有的很亮，有的偏黄，有的藏在阴影里。小芽把它们都记住了。
      </p>
      <button class="primary share-button" @click="go('/share')">分享</button>
    </article>
    <FigmaBottomNav active="diary" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
