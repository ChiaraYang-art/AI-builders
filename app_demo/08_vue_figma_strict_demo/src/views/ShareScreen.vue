<script setup>
import { useAppNavigation } from "../composables/useAppNavigation.js";
import { useShareSheet } from "../composables/useShareSheet.js";
import { asset } from "../utils/assets.js";

const { go } = useAppNavigation();
const { toast, shareSheet, saveShare, openShare } = useShareSheet();
</script>

<template>
  <section class="screen">
    <p class="time">9:41</p>
    <button type="button" class="back" @click="go('/diary/essay')" aria-label="返回">‹</button>
    <h1 class="title with-back">分享卡片</h1>
    <p class="subtitle with-back">把今天的小芽日记发给朋友</p>
    <article class="share-card">
      <img :src="asset('share-card-sprout.svg')" alt="分享卡片插画" />
      <h2>今天我听见了春天</h2>
      <p>32 min walk · 3 sounds · 5 greens</p>
      <span>刚才的光不是屏幕里的光。它落在我的叶子上，也落在你的鞋尖前面。</span>
    </article>
    <div class="share-actions">
      <button class="primary" @click="saveShare">保存图片</button>
      <button class="secondary" @click="openShare">分享</button>
    </div>
    <article class="privacy-card">
      <strong>隐私提示</strong>
      <span>分享卡片默认不包含精确位置。</span>
    </article>
    <div v-if="toast" class="toast">{{ toast }}</div>
    <div v-if="shareSheet" class="share-sheet">
      <h3>分享到</h3>
      <div><button>小红书</button><button>微信</button><button>朋友圈</button></div>
    </div>
  </section>
</template>
