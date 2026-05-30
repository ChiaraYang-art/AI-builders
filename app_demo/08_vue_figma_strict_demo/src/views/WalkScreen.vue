<script setup>
import { computed, ref } from "vue";

import { buildWalkSensorLines, splitSpeechLines } from "../api/sprout.js";
import { buildWalkMediaUrl } from "../api/walk.js";
import FigmaBottomNav from "../components/FigmaBottomNav.vue";
import LiveSensorList from "../components/LiveSensorList.vue";
import {
  useAppNavigation,
  useSproutLiveContext,
  useWalkSessionContext,
} from "../composables/useAppNavigation.js";
import { useWalkTimer } from "../composables/useWalkTimer.js";
import { asset } from "../utils/assets.js";

const props = defineProps({
  type: {
    type: String,
    required: true,
  },
});

const photoInput = ref(null);

const { latest } = useSproutLiveContext();
const { go, randomInvite, inviteForType } = useAppNavigation();
const walkSession = useWalkSessionContext();
const { walkTime } = useWalkTimer();

const invite = computed(() => inviteForType(props.type));
const sensorLines = computed(() => buildWalkSensorLines(latest.value));
const speechLines = computed(() => {
  const live = splitSpeechLines(latest.value?.speech_full || latest.value?.speech);
  if (live[0] && latest.value?.speech) {
    return live;
  }

  return invite.value.speech;
});
const pausePath = computed(() => `/pause/${props.type}`);
const activeWalk = computed(() => walkSession.activeWalk.value || latest.value?.active_walk);
const colorProgress = computed(() => activeWalk.value?.color_progress || { current: 0, target: 5, colors: [] });
const photoItems = computed(() => activeWalk.value?.photos || []);
const audioCount = computed(() => activeWalk.value?.audios?.length || 0);
const colorComplete = computed(
  () => props.type === "color" && colorProgress.value.current >= colorProgress.value.target,
);

function openPhotoPicker() {
  photoInput.value?.click();
}

async function onPhotoSelected(event) {
  const file = event.target.files?.[0];
  event.target.value = "";

  if (!file) {
    return;
  }

  try {
    await walkSession.uploadPhoto(file);
  } catch {
    // uploadError shown in template
  }
}

function toggleRecording() {
  if (walkSession.isRecording.value) {
    walkSession.stopRecording();
    return;
  }

  walkSession.startRecording();
}
</script>

<template>
  <section class="screen">
    <p class="time">9:41</p>
    <button type="button" class="back" @click="go(pausePath)" aria-label="返回">‹</button>
    <h1 class="title with-back">{{ invite.walkTitle }}</h1>
    <p class="subtitle with-back">{{ invite.walkSubtitle }}</p>
    <LiveSensorList :lines="sensorLines" extra-class="walk-sensors" />
    <p class="walk-time">{{ walkTime }}</p>
    <article class="speech-card walk-speech">
      <span>{{ speechLines[0] }}</span>
      <span v-if="speechLines[1]">{{ speechLines[1] }}</span>
    </article>

    <div v-if="type === 'color'" class="walk-upload-panel color-walk-panel">
      <div class="walk-upload-head">
        <strong>收集绿色 {{ colorProgress.current }}/{{ colorProgress.target }}</strong>
        <span v-if="colorProgress.colors.length">{{ colorProgress.colors.join(" · ") }}</span>
      </div>
      <div class="color-photo-grid live-photo-grid">
        <img
          v-for="(photo, index) in photoItems"
          :key="photo.filename || index"
          :src="buildWalkMediaUrl(photo.url)"
          alt="路上拍到的颜色"
        />
        <button
          v-if="photoItems.length < 6"
          type="button"
          class="photo-slot add-photo"
          :disabled="walkSession.isUploading.value || colorComplete"
          @click="openPhotoPicker"
        >
          {{ walkSession.isUploading.value ? "上传中…" : "+" }}
        </button>
      </div>
      <input
        ref="photoInput"
        type="file"
        accept="image/*"
        capture="environment"
        class="hidden-input"
        @change="onPhotoSelected"
      />
    </div>

    <div v-else-if="type === 'sound'" class="walk-upload-panel sound-walk-panel">
      <img class="sound-graphic" :src="asset('sound-graphic.svg')" alt="声音图形" />
      <p class="sound-count">已记录 {{ audioCount }} 段环境音</p>
      <button
        type="button"
        class="primary record-button"
        :disabled="walkSession.isUploading.value"
        @click="toggleRecording"
      >
        {{
          walkSession.isRecording.value
            ? "停止录音"
            : walkSession.isUploading.value
              ? "上传中…"
              : "录一段环境音"
        }}
      </button>
    </div>

    <img v-else class="sprout sprout-walk" :src="asset('sprout-happy.svg')" alt="开心的小芽" />

    <p v-if="walkSession.uploadError.value" class="walk-upload-error">
      {{ walkSession.uploadError.value }}
    </p>

    <FigmaBottomNav
      active="walk"
      :home-path="pausePath"
      @navigate="go"
      @random-invite="randomInvite"
    />
  </section>
</template>
