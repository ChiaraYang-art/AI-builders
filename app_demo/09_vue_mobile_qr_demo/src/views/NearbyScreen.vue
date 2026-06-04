<script setup>
import FigmaBottomNav from "../components/FigmaBottomNav.vue";
import { useAppNavigation } from "../composables/useAppNavigation.js";
import { asset } from "../utils/assets.js";

const { go, randomInvite } = useAppNavigation();

const nearbyCards = [
  {
    image: "nearby pic 1.svg",
    title: "新开的花店",
    text: "就在四平路附近",
  },
  {
    image: "nearby pic 2.svg",
    title: "墙角多了一丛薄荷",
    text: "我每天中午去公园摸鱼坐的那个长椅后面多了一丛薄荷",
    tall: true,
  },
  {
    image: "nearby pic 3.svg",
    title: "有人拍到了晚霞",
    text: "上海 la la land 的含金量",
  },
  {
    image: "nearby pic 4.svg",
    title: "树友发现蓝色窗台",
    text: "好漂亮的蓝色",
  },
  {
    image: "nearby pic 5.svg",
    title: "树影落在老房子上",
    text: "下午会很好看",
  },
  {
    image: "nearby pic 6.svg",
    title: "门口有新的绿植",
    text: "可以下次去看看",
  },
];
</script>

<template>
  <section class="screen nearby-screen">
    <p class="time">9:41</p>
    <h1 class="nearby-title">附近</h1>
    <p class="nearby-subtitle">看看今天城市长出了什么新东西</p>

    <div class="nearby-tabs">
      <button type="button" class="active">附近新店</button>
      <button type="button">新变化</button>
      <button type="button">树友发现</button>
    </div>

    <div class="nearby-grid">
      <article
        v-for="card in nearbyCards"
        :key="card.title"
        class="nearby-place-card"
        :class="{ tall: card.tall }"
      >
        <img :src="asset(card.image)" :alt="card.title" />
        <h2>{{ card.title }}</h2>
        <p>{{ card.text }}</p>
      </article>
    </div>

    <FigmaBottomNav active="nearby" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
