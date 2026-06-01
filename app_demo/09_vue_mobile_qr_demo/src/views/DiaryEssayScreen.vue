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
const titleText = computed(() => {
  if (diary.value?.title) {
    return diary.value.title;
  }

  return isColor.value ? "今天我收集了很多绿色" : "今天我听见了春天";
});
const essayParagraphs = computed(() => {
  if (diary.value?.essay) {
    return diary.value.essay
      .split(/\n+/)
      .map((line) => line.trim())
      .filter(Boolean);
  }

  if (latest.value?.speech_full) {
    return [latest.value.speech_full];
  }

  if (isColor.value) {
    return [
      "我们从房间里出发，先遇见了树影，然后遇见了草丛、路牌和店门口的小盆栽。",
      "这些绿色不是一种颜色。它们有的很亮，有的偏黄，有的藏在阴影里。小芽把它们都记住了。",
    ];
  }

  return [
    "今天我们走到了有树影的路边。阳光先落在我的叶子上，然后慢慢变暖。",
    "我听见了车轮、脚步和风。你拍下的绿色像新的叶尖，提醒我真实世界一直在悄悄长大。",
  ];
});
const photoItems = computed(() => diary.value?.photos || []);
</script>

<template>
  <section class="screen scroll-screen diary-essay-screen">
    <p class="time">9:41</p>
    <button type="button" class="back" @click="go('/diary')" aria-label="返回">‹</button>
    <h1 class="title with-back">小芽日记</h1>
    <p class="subtitle with-back">{{ isColor ? "Color Walk 小作文" : "今日散步小作文" }}</p>
    <div class="tabs">
      <button @click="go(isColor ? '/diary/log/color' : '/diary/log')">流水账</button>
      <button class="active">小作文</button>
    </div>
    <article class="diary-card essay">
      <img
        v-if="!isColor"
        class="essay-sprout-visual"
        :src="asset('diary-page-sprout.svg')"
        alt="日记页小芽插画"
      />
      <h2>{{ titleText }}</h2>
      <p v-for="paragraph in essayParagraphs" :key="paragraph">{{ paragraph }}</p>
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
      <button class="primary share-button" @click="go('/share')">分享</button>
    </article>
    <FigmaBottomNav active="diary" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
