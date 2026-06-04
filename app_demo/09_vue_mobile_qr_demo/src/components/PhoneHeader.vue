<script setup>
defineProps({
  title: {
    type: String,
    required: true,
  },
  subtitle: {
    type: String,
    default: "",
  },
  backRoute: {
    type: String,
    default: "",
  },
});

const emit = defineEmits(["go"]);
</script>

<template>
  <p class="time">9:41</p>
  <button v-if="backRoute" type="button" class="back" aria-label="返回" @click="emit('go', backRoute)">
    ‹
  </button>
  <h1 class="title" :class="{ 'with-back': backRoute }">{{ title }}</h1>
  <p v-if="subtitle" class="subtitle" :class="{ 'with-back': backRoute }">{{ subtitle }}</p>
</template>
