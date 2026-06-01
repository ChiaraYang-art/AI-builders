<script setup>
import { provide, watch } from "vue";
import { RouterView } from "vue-router";

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
    <section class="phone-shell">
      <section class="phone" aria-label="出走小芽手机扫码访问版">
        <RouterView />
      </section>
    </section>
  </main>
</template>
