<script setup>
defineProps({
  outdoor: {
    type: Boolean,
    default: false,
  },
});
</script>

<template>
  <ul class="sensor">
    <li>
      <strong>{{ outdoor ? "在室外" : "在室内" }}</strong>
    </li>
    <li>{{ outdoor ? "光照充足" : "光照偏低" }}</li>
    <li>{{ outdoor ? "有城市的声音" : "很安静" }}</li>
    <li>温度28.3°C 湿度43.1%</li>
  </ul>
</template>
