import { createRouter, createWebHashHistory } from "vue-router";

import AtlasScreen from "../views/AtlasScreen.vue";
import DiaryEssayScreen from "../views/DiaryEssayScreen.vue";
import DiaryLogScreen from "../views/DiaryLogScreen.vue";
import DiaryScreen from "../views/DiaryScreen.vue";
import FinishScreen from "../views/FinishScreen.vue";
import HomeScreen from "../views/HomeScreen.vue";
import InviteScreen from "../views/InviteScreen.vue";
import MapScreen from "../views/MapScreen.vue";
import MeScreen from "../views/MeScreen.vue";
import NearbyScreen from "../views/NearbyScreen.vue";
import PauseScreen from "../views/PauseScreen.vue";
import ShareScreen from "../views/ShareScreen.vue";
import WalkScreen from "../views/WalkScreen.vue";

const router = createRouter({
  history: createWebHashHistory(),
  routes: [
    { path: "/", redirect: "/home" },
    { path: "/home", name: "home", component: HomeScreen },
    { path: "/invite/:type", name: "invite", component: InviteScreen, props: true },
    { path: "/walk/:type", name: "walk", component: WalkScreen, props: true },
    { path: "/pause/:type", name: "pause", component: PauseScreen, props: true },
    { path: "/finish/:variant?", name: "finish", component: FinishScreen, props: true },
    { path: "/diary", name: "diary", component: DiaryScreen },
    { path: "/diary/log/:variant?", name: "diary-log", component: DiaryLogScreen, props: true },
    { path: "/diary/essay/:variant?", name: "diary-essay", component: DiaryEssayScreen, props: true },
    { path: "/map", name: "map", component: MapScreen },
    { path: "/nearby", redirect: "/home" },
    { path: "/me", name: "me", component: MeScreen },
    { path: "/atlas", name: "atlas", component: AtlasScreen },
    { path: "/share", name: "share", component: ShareScreen },
  ],
});

const demoWalkTypes = new Set(["light", "color"]);

router.beforeEach((to) => {
  const walkType = to.params.type;
  if (typeof walkType === "string" && !demoWalkTypes.has(walkType)) {
    if (to.name === "invite" || to.name === "walk" || to.name === "pause") {
      return { path: `/invite/light` };
    }
  }
  return true;
});

export default router;
