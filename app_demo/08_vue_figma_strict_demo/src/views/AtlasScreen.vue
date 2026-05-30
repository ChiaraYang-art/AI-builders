<script setup>
import { computed } from "vue";

import { atlasItems } from "../constants/demo.js";
import FigmaBottomNav from "../components/FigmaBottomNav.vue";
import { useAppNavigation, useSproutLiveContext } from "../composables/useAppNavigation.js";
import { asset } from "../utils/assets.js";

const { latest } = useSproutLiveContext();
const { go, randomInvite } = useAppNavigation();

const unlockedSet = computed(() => new Set(latest.value?.atlas_unlocked || []));

function statusFor(name) {
  return unlockedSet.value.has(name) ? "已解锁" : "去散步解锁";
}
</script>

<template>
  <section class="screen">
    <p class="time">9:41</p>
    <button type="button" class="back" @click="go('/me')" aria-label="返回">‹</button>
    <h1 class="title with-back">小芽图鉴</h1>
    <p class="subtitle with-back">收集不同的小芽形态</p>
    <div class="atlas-grid">
      <article
        v-for="[name, file] in atlasItems"
        :key="name"
        class="atlas-card"
        :class="{ locked: !unlockedSet.has(name) }"
      >
        <img :src="asset(file)" :alt="name" />
        <h3>{{ name }}</h3>
        <p>{{ statusFor(name) }}</p>
      </article>
    </div>
    <FigmaBottomNav active="me" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
