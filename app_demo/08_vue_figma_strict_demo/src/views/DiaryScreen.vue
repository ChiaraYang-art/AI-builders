<script setup>
import FigmaBottomNav from "../components/FigmaBottomNav.vue";
import { useAppNavigation } from "../composables/useAppNavigation.js";
import { dayClass } from "../utils/demoHelpers.js";
import { asset } from "../utils/assets.js";

const { go, randomInvite } = useAppNavigation();
</script>

<template>
  <section class="screen">
    <p class="time">9:41</p>
    <h1 class="title">小芽日记</h1>
    <p class="subtitle">每天的小天气，都会长成记忆</p>
    <article class="calendar-card" @click="go('/diary/log')">
      <div class="calendar-title"><span>‹</span><strong>May 2026</strong><span>›</span></div>
      <p>12 walks · 5 sunny days · 3 new sounds</p>
      <div class="achievement">成就：城市探索家</div>
      <div class="calendar-grid">
        <span v-for="day in ['M', 'T', 'W', 'T', 'F', 'S', 'S']" :key="day" class="weekday">{{ day }}</span>
        <span></span><span></span>
        <span v-for="day in 31" :key="day" :class="dayClass(day)">{{ day }}</span>
      </div>
    </article>
    <article class="map-card" @click="go('/map')">
      <div>
        <h2>散步地图</h2>
        <p>在地图上查看这个月和小芽一起留下的散步记忆</p>
      </div>
      <img :src="asset('walk-map-card.svg')" alt="散步地图预览" />
    </article>
    <FigmaBottomNav active="diary" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
