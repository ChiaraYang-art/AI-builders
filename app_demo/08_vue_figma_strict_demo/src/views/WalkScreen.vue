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
const displaySensorLines = computed(() => {
  if (props.type !== "sound") {
    return sensorLines.value;
  }

  return ["在室外", "光照充足", "有城市的声音", "温度28.3°C 湿度43.1%"];
});
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
const audioItems = computed(() => activeWalk.value?.audios || []);
const audioCount = computed(() => audioItems.value.length);
const colorComplete = computed(
  () => props.type === "color" && colorProgress.value.current >= colorProgress.value.target,
);

const soundEvents = computed(() => {
  if (audioItems.value.length > 0) {
    return audioItems.value.slice(-2).map((audio, index) => ({
      time: audio.created_at?.slice(11, 16) || (index === 0 ? "15:31" : "15:41"),
      metric: audio.summary || "range 0.42 · variance 0.18 · lively",
      comment: audio.comment || (index === 0 ? "我好像听到了鸟鸣！" : "这座城市今天很热闹。"),
    }));
  }

  return [
    {
      time: "15:31",
      metric: "range 0.42 · variance 0.18 · lively",
      comment: "我好像听到了鸟鸣！",
    },
    {
      time: "15:41",
      metric: "range 0.42 · variance 0.18 · lively",
      comment: "这座城市今天很热闹。",
    },
  ];
});

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
  <section class="screen" :class="{ 'sound-walk-screen': type === 'sound' }">
    <p class="time">9:41</p>
    <button type="button" class="back" @click="go(pausePath)" aria-label="返回">‹</button>
    <h1 class="title with-back">{{ invite.walkTitle }}</h1>
    <p class="subtitle with-back">
      {{ type === "sound" ? "小芽想听见城市的起伏" : invite.walkSubtitle }}
    </p>
    <LiveSensorList :lines="displaySensorLines" extra-class="walk-sensors" />
    <p class="walk-time">{{ walkTime }}</p>

    <article v-if="type !== 'sound'" class="speech-card walk-speech">
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
          {{ walkSession.isUploading.value ? "上传中" : "+" }}
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

    <div v-else-if="type === 'sound'" class="sound-timeline">
      <div v-for="(event, index) in soundEvents" :key="`${event.time}-${index}`" class="sound-event">
        <button
          type="button"
          class="sound-event-card"
          :disabled="walkSession.isUploading.value"
          @click="toggleRecording"
        >
          <span class="sound-time">{{ walkSession.isRecording.value && index === 0 ? "录音中" : event.time }}</span>
          <span class="sound-bars" aria-hidden="true">
            <i v-for="bar in 16" :key="bar"></i>
          </span>
          <strong>{{ walkSession.isUploading.value && index === 0 ? "uploading audio..." : event.metric }}</strong>
        </button>
        <div class="sprout-comment">
          <img :src="asset('sprout-comment.svg')" alt="" />
          <span>{{ event.comment }}</span>
        </div>
      </div>
    </div>

    <img v-else class="sprout sprout-walk" :src="asset(invite.sprout)" alt="散步中的小芽" />

    <p v-if="type === 'sound'" class="sound-record-note">
      已记录 {{ audioCount }} 段环境音。点击声音卡片可{{ walkSession.isRecording.value ? "停止录音" : "录一段环境音" }}。
    </p>

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
