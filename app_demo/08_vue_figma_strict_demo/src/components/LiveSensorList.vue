<script setup>
defineProps({
  lines: {
    type: Array,
    required: true,
  },
  extraClass: {
    type: String,
    default: "",
  },
});
</script>

<template>
  <ul class="sensor-list" :class="extraClass">
    <li v-for="(line, index) in lines" :key="`${extraClass}-${index}`">
      <strong v-if="index === 0">{{ line }}</strong>
      <template v-else>{{ line }}</template>
    </li>
  </ul>
</template>
