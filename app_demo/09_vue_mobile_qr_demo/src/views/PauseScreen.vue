<script setup>
import { computed, ref } from "vue";

import { useAppNavigation } from "../composables/useAppNavigation.js";

const props = defineProps({
  type: {
    type: String,
    required: true,
  },
});

const isSubmitting = ref(false);

const { go, finishWalk, finishRouteFor, inviteForType } = useAppNavigation();

const invite = computed(() => inviteForType(props.type));

async function completeWalk() {
  if (isSubmitting.value) {
    return;
  }

  isSubmitting.value = true;

  try {
    await finishWalk(props.type);
  } catch {
    isSubmitting.value = false;
  }
}
</script>

<template>
  <section class="screen">
    <p class="time">9:41</p>
    <h1 class="title">散步暂停</h1>
    <p class="subtitle">可以继续，也可以先回去</p>
    <article class="pause-card">
      <p>中途退出</p>
      <h2>要先休息一下吗？</h2>
      <span>小芽会保留刚才收集到的光、声音和移动记录。</span>
      <div class="actions">
        <button type="button" class="primary" @click="go(`/walk/${type}`)">继续散步</button>
        <button type="button" class="secondary" :disabled="isSubmitting" @click="completeWalk">
          {{ isSubmitting ? "生成日记中…" : "完成散步" }}
        </button>
      </div>
    </article>
  </section>
</template>
