<script setup>
const props = defineProps({
  active: {
    type: String,
    default: "home",
  },
  homeRoute: {
    type: String,
    default: "home",
  },
});

const emit = defineEmits(["go", "randomInvite"]);

const items = [
  { key: "home", label: "首页" },
  { key: "walk", label: "散步" },
  { key: "diary", label: "日记" },
  { key: "nearby", label: "附近" },
  { key: "me", label: "我的" },
];

function handleClick(key) {
  if (key === "walk") {
    emit("randomInvite");
    return;
  }

  if (key === "home") {
    emit("go", props.homeRoute);
    return;
  }

  emit("go", key);
}
</script>

<template>
  <nav class="bottom-nav" aria-label="底部导航">
    <button
      v-for="item in items"
      :key="item.key"
      type="button"
      class="nav-item"
      :class="{ active: active === item.key }"
      @click="handleClick(item.key)"
    >
      <span class="nav-icon" aria-hidden="true">{{ item.label.slice(0, 1) }}</span>
      {{ item.label }}
    </button>
  </nav>
</template>
