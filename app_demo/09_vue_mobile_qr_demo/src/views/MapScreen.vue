<script setup>
import { computed } from "vue";

import { sproutAssetForState } from "../api/sprout.js";
import FigmaBottomNav from "../components/FigmaBottomNav.vue";
import { useAppNavigation, useSproutLiveContext } from "../composables/useAppNavigation.js";
import { asset } from "../utils/assets.js";

const { latest } = useSproutLiveContext();
const { go, randomInvite } = useAppNavigation();

const diary = computed(() => latest.value?.walk_diary);
const mapSproutAsset = computed(() => sproutAssetForState(latest.value?.state || "walking"));
const mapSummary = computed(() => {
  if (diary.value?.map_summary) {
    return diary.value.map_summary;
  }

  return "1.8km · 4 个发现 · 阳光很足";
});
</script>

<template>
  <section class="screen map-screen">
    <p class="time">9:41</p>
    <button type="button" class="back" @click="go('/diary')" aria-label="返回">‹</button>
    <h1 class="title with-back">小芽的散步地图</h1>
    <p class="subtitle with-back">我们一起去了什么地方？</p>
    <img class="walk-map" :src="asset('walk-map.svg')" alt="散步地图" />
    <article class="map-summary">
      <div>
        <strong>今天的小芽散步路线</strong>
        <span>{{ mapSummary }}</span>
      </div>
      <img :src="asset(mapSproutAsset)" alt="小芽" />
    </article>
    <FigmaBottomNav active="diary" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
