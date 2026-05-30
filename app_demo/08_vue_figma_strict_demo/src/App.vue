<script setup>
import { provide, watch } from "vue";
import { RouterView } from "vue-router";

import DemoNotesPanel from "./components/DemoNotesPanel.vue";
import { SPRout_LIVE_KEY } from "./composables/useAppNavigation.js";
import { useSproutLive } from "./composables/useSproutLive.js";
import { useWalkSession, WALK_SESSION_KEY } from "./composables/useWalkSession.js";

const sproutLive = useSproutLive(4000);
const walkSession = useWalkSession(sproutLive.refresh);

provide(SPRout_LIVE_KEY, sproutLive);
provide(WALK_SESSION_KEY, walkSession);

watch(
  () => sproutLive.latest.value,
  (latest) => {
    walkSession.syncFromLatest(latest);
  },
  { immediate: true },
);
</script>

<template>
  <main class="demo-page">
    <section class="phone" aria-label="出走小芽 Figma 优先 Vue Demo">
      <RouterView />
    </section>
    <DemoNotesPanel />
  </main>
</template>
