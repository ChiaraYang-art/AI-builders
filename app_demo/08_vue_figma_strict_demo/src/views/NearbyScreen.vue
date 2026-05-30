<script setup>
import FigmaBottomNav from "../components/FigmaBottomNav.vue";
import { useAppNavigation } from "../composables/useAppNavigation.js";
import { asset } from "../utils/assets.js";

const { go, randomInvite } = useAppNavigation();
</script>

<template>
  <section class="screen">
    <p class="time">9:41</p>
    <h1 class="title">附近</h1>
    <p class="subtitle">城市里新长出来的线索</p>
    <article class="invite-card nearby-card">
      <p>Local Discovery</p>
      <h2>国权路的咖啡店换了新菜单。如果我们走路过去，还能看看路边的银杏。</h2>
    </article>
    <img class="sprout sprout-invite" :src="asset('sprout-wilted.svg')" alt="小芽" />
    <FigmaBottomNav active="nearby" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
