<script setup>
import { computed } from "vue";

import {
  buildSensorLines,
  formatUpdatedAt,
  splitSpeechLines,
  sproutAssetForState,
  stateTitle,
} from "../api/sprout.js";
import FigmaBottomNav from "../components/FigmaBottomNav.vue";
import LiveSensorList from "../components/LiveSensorList.vue";
import { useAppNavigation, useSproutLiveContext } from "../composables/useAppNavigation.js";
import { asset } from "../utils/assets.js";

const { latest, audioEnabled, enableAudio } = useSproutLiveContext();
const { go, randomInvite, smartInvite } = useAppNavigation();

const homeTitle = computed(() => stateTitle(latest.value?.state));
const homeSubtitle = computed(() => formatUpdatedAt(latest.value?.updated_at));
const homeSensorLines = computed(() => buildSensorLines(latest.value));
const homeSpeechLines = computed(() =>
  splitSpeechLines(latest.value?.speech_full || latest.value?.speech)
);
const homeSproutAsset = computed(() => sproutAssetForState(latest.value?.state || "idle"));

function handleSpeechClick() {
  smartInvite(latest.value);
}
</script>

<template>
  <section class="screen">
    <p class="time">9:41</p>
    <h1 class="title">{{ homeTitle }}</h1>
    <p class="subtitle">{{ homeSubtitle }}</p>
    <LiveSensorList :lines="homeSensorLines" />
    <button type="button" class="speech-card home-speech" @click="handleSpeechClick">
      <span>{{ homeSpeechLines[0] }}</span>
      <span v-if="homeSpeechLines[1]">{{ homeSpeechLines[1] }}</span>
    </button>
    <button v-if="!audioEnabled" type="button" class="audio-enable" @click="enableAudio">
      开启语音
    </button>
    <img class="sprout sprout-home" :src="asset(homeSproutAsset)" :alt="homeTitle" />
    <FigmaBottomNav active="home" @navigate="go" @random-invite="randomInvite" />
  </section>
</template>
