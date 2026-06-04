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
const skipPromptVisible = ref(false);

const { go, finishWalk, finishRouteFor, inviteForType } = useAppNavigation();

const invite = computed(() => inviteForType(props.type));
const skipPrompt = computed(() => {
  if (props.type === "color") {
    return {
      message: "还没收集到颜色呢，今天要先跳过嘛？",
      action: "确定跳过",
    };
  }

  return {
    message: "好像还没接收到足够的光呢，现在就要回去嘛？",
    action: "仍然结束",
  };
});

function isWalkNotComplete(error) {
  return error?.message === "walk_not_complete" || error?.payload?.error === "walk_not_complete";
}

async function completeWalk(options = {}) {
  if (isSubmitting.value) {
    return;
  }

  isSubmitting.value = true;

  try {
    await finishWalk(props.type, options);
  } catch (error) {
    if (!options.forceComplete && (props.type === "light" || props.type === "color") && isWalkNotComplete(error)) {
      skipPromptVisible.value = true;
    }
  } finally {
    isSubmitting.value = false;
  }
}

async function forceCompleteWalk() {
  if (isSubmitting.value) {
    return;
  }

  isSubmitting.value = true;

  try {
    await finishWalk(props.type, { forceComplete: true });
  } catch {
    go(finishRouteFor(props.type));
  } finally {
    isSubmitting.value = false;
    skipPromptVisible.value = false;
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
        <button type="button" class="secondary" :disabled="isSubmitting" @click="completeWalk()">
          {{ isSubmitting ? "生成日记中..." : "完成散步" }}
        </button>
      </div>
    </article>

    <div v-if="skipPromptVisible" class="skip-modal-backdrop">
      <article class="skip-modal">
        <p>{{ invite.walkTitle }}</p>
        <h2>{{ skipPrompt.message }}</h2>
        <div class="skip-modal-actions">
          <button type="button" class="secondary" @click="skipPromptVisible = false">再走一会儿</button>
          <button type="button" class="primary" :disabled="isSubmitting" @click="forceCompleteWalk">
            {{ isSubmitting ? "生成日记中..." : skipPrompt.action }}
          </button>
        </div>
      </article>
    </div>
  </section>
</template>
