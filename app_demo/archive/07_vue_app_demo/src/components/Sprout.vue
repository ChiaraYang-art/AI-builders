<script setup>
defineProps({
  mood: {
    type: String,
    default: "wilted",
  },
  variant: {
    type: String,
    default: "",
  },
});
</script>

<template>
  <div class="sprout" :class="[mood === 'happy' ? 'happy' : '', variant]" aria-label="小芽形象">
    <div class="sprout-body">
      <span v-for="index in 5" :key="index" class="soil"></span>
      <span class="eye left"></span>
      <span class="eye right"></span>
    </div>
    <span v-for="name in ['one', 'two', 'three']" :key="name" class="flower" :class="name">
      <i v-for="index in 5" :key="index" class="petal"></i>
    </span>
  </div>
</template>
