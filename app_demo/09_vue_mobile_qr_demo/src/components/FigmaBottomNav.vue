<script setup>
import { navIcons } from "../constants/demo.js";
import { asset } from "../utils/assets.js";

const emit = defineEmits(["navigate", "randomInvite"]);

const props = defineProps({
  active: {
    type: String,
    required: true,
  },
  homePath: {
    type: String,
    default: "/home",
  },
});

const navItems = [
  { key: "home", label: "首页" },
  { key: "walk", label: "散步" },
  { key: "diary", label: "日记" },
  { key: "nearby", label: "附近" },
  { key: "me", label: "我的" },
];

function handleClick(item) {
  if (item.key === "walk") {
    emit("randomInvite");
    return;
  }

  if (item.key === "home") {
    emit("navigate", props.homePath);
    return;
  }

  emit("navigate", `/${item.key}`);
}

function iconFor(item) {
  const icons = navIcons[item.key];
  return asset(icons[item.key === props.active ? "selected" : "normal"]);
}
</script>

<template>
  <nav class="bottom-nav">
    <button
      v-for="item in navItems"
      :key="item.key"
      type="button"
      class="nav-item"
      :class="{ active: active === item.key }"
      @click="handleClick(item)"
    >
      <img :src="iconFor(item)" alt="" />
      <span>{{ item.label }}</span>
    </button>
  </nav>
</template>
