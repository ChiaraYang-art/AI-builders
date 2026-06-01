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
const luxText = computed(() => {
  const lux = latest.value?.lux;
  if (lux === null || lux === undefined || lux < 0) {
    return "lux 数据等待硬件上报";
  }

  return `当前 lux ${Number(lux).toFixed(0)}，小芽正在感知周围亮度。`;
});
const timelineItems = computed(() => {
  if (diary.value?.timeline?.length) {
    return diary.value.timeline.map((line) => {
      const match = line.match(/^(\d{1,2}:\d{2})\s+(.*)$/);
      if (match) {
        return { label: match[1], text: match[2] };
      }

      return { label: "记录", text: line };
    });
  }

  return [
    { label: "实时", text: luxText.value },
    { label: "15:08", text: "声音波动变大，可能经过了路口和树下。" },
    { label: "15:18", text: "银杏、草丛、店门口的小盆栽、路牌和树影。" },
  ];
});
const photoItems = computed(() => diary.value?.photos || []);
</script>

<template>
  <section class="screen">
    <p class="time">9:41</p>
    <button type="button" class="back" @click="go('/diary')" aria-label="返回">‹</button>
    <h1 class="title with-back">小芽日记</h1>
    <p class="subtitle with-back">{{ isColor ? "Color Walk 记录" : "2026 年 5 月" }}</p>
    <div class="tabs">
      <button class="active">流水账</button>
      <button @click="go(isColor ? '/diary/essay/color' : '/diary/essay')">小作文</button>
    </div>
    <article class="diary-card">
      <h2>{{ diary?.title || (isColor ? "绿色收集记录" : "今天的小天气") }}</h2>
      <p>{{ isColor ? "按顺序记录路上遇到的颜色。" : "按时间记录传感器看到的变化。" }}</p>
      <div v-if="photoItems.length" class="color-photo-grid diary-photo-grid">
        <img
          v-for="(photo, index) in photoItems"
          :key="photo.filename || index"
          :src="buildWalkMediaUrl(photo.url)"
          alt="散步照片"
        />
      </div>
      <img
        v-else-if="isColor"
        class="diary-color-photos"
        :src="asset('diary-color-walk-photos.svg')"
        alt="Color Walk 照片记录"
      />
      <img v-else class="diary-sprout" :src="asset('diary-page-sprout.svg')" alt="日记小芽" />
      <div class="log-list">
        <div v-for="(item, index) in timelineItems" :key="index" class="log-item">
          <strong>{{ item.label }}</strong>
          <span>{{ item.text }}</span>
        </div>
      </div>
    </article>
    <FigmaBottomNav active="diary" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
