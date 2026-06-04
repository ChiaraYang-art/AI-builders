import { createApp } from "vue";

import App from "./App.vue";
import router from "./router/index.js";
import "./styles.css";

const setPhoneScale = () => {
  const width = window.visualViewport?.width ?? window.innerWidth;
  const height = window.visualViewport?.height ?? window.innerHeight;
  const safeWidth = Math.max(width - 16, 320);
  const safeHeight = Math.max(height - 16, 520);
  const scale = Math.min(safeWidth / 393, safeHeight / 852, 1);
  document.documentElement.style.setProperty("--phone-scale", scale.toFixed(4));
  document.documentElement.style.setProperty("--phone-shell-w", `${393 * scale}px`);
  document.documentElement.style.setProperty("--phone-shell-h", `${852 * scale}px`);
};

setPhoneScale();
window.addEventListener("resize", setPhoneScale, { passive: true });
window.visualViewport?.addEventListener("resize", setPhoneScale, { passive: true });

createApp(App).use(router).mount("#app");
