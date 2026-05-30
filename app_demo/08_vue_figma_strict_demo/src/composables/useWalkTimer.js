import { computed, onBeforeUnmount, ref, watch } from "vue";
import { useRoute } from "vue-router";

let timerId = null;
const seconds = ref(182);

export function resetWalkSeconds() {
  seconds.value = 182;
}

export function useWalkTimer() {
  const route = useRoute();

  const walkTime = computed(() => {
    const minutes = String(Math.floor(seconds.value / 60)).padStart(2, "0");
    const secs = String(seconds.value % 60).padStart(2, "0");
    return `已散步时间 0:${minutes}:${secs}`;
  });

  function stopTimer() {
    if (timerId) {
      window.clearInterval(timerId);
      timerId = null;
    }
  }

  function startTimer(reset = false) {
    if (reset) {
      seconds.value = 182;
    }

    stopTimer();
    timerId = window.setInterval(() => {
      seconds.value += 1;
    }, 1000);
  }

  watch(
    () => route.path,
    (path) => {
      if (path.startsWith("/walk/")) {
        if (!timerId) {
          startTimer(false);
        }
      } else {
        stopTimer();
      }
    },
    { immediate: true }
  );

  onBeforeUnmount(stopTimer);

  return {
    seconds,
    walkTime,
    startTimer,
    stopTimer,
  };
}
