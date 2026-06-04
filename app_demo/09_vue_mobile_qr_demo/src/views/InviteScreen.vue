<script setup>
import { computed, ref } from "vue";

import { buildSensorLines, splitSpeechLines } from "../api/sprout.js";
import FigmaBottomNav from "../components/FigmaBottomNav.vue";
import LiveSensorList from "../components/LiveSensorList.vue";
import { useAppNavigation, useSproutLiveContext } from "../composables/useAppNavigation.js";
import { asset } from "../utils/assets.js";

const props = defineProps({
  type: {
    type: String,
    required: true,
  },
});

const isStarting = ref(false);

const { latest } = useSproutLiveContext();
const { go, randomInvite, startWalk, secondaryInviteRoute, inviteForType } = useAppNavigation();

const invite = computed(() => inviteForType(props.type));
const sensorLines = computed(() => buildSensorLines(latest.value));
const inviteHint = computed(() => {
  const lines = splitSpeechLines(latest.value?.speech_short || latest.value?.speech);
  return lines[0] || invite.value.title;
});

async function beginWalk() {
  if (isStarting.value) {
    return;
  }

  isStarting.value = true;

  try {
    await startWalk(props.type);
  } catch {
    isStarting.value = false;
  }
}
</script>

<template>
  <section class="screen">
    <p class="time">9:41</p>
    <h1 class="title">出门邀请</h1>
    <p class="subtitle">{{ invite.subtitle }}</p>
    <LiveSensorList :lines="sensorLines" />
    <article class="invite-card" :class="[`invite-card--${type}`, { local: invite.local }]">
      <p>{{ invite.label }}</p>
      <h2>{{ inviteHint }}</h2>
      <div class="actions">
        <button type="button" class="primary" :disabled="isStarting" @click="beginWalk">
          {{ isStarting ? "准备中…" : "出门！" }}
        </button>
        <button type="button" class="secondary" @click="go(secondaryInviteRoute(type))">
          {{ type === "light" || type === "sound" ? "今天算了吧" : "换个更轻松的任务" }}
        </button>
      </div>
    </article>
    <img class="sprout sprout-invite" :src="asset(invite.sprout)" alt="小芽" />
    <FigmaBottomNav active="walk" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
