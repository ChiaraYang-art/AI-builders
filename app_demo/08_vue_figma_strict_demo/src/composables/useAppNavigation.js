import { inject } from "vue";
import { useRouter } from "vue-router";

import { suggestInviteType } from "../api/sprout.js";
import { inviteTypes, invites, LEGACY_ROUTE_MAP } from "../constants/demo.js";
import { resetWalkSeconds } from "./useWalkTimer.js";
import { WALK_SESSION_KEY } from "./useWalkSession.js";

export const SPRout_LIVE_KEY = Symbol("sproutLive");

export function useSproutLiveContext() {
  const context = inject(SPRout_LIVE_KEY);

  if (!context) {
    throw new Error("sproutLive context is not available.");
  }

  return context;
}

export function useWalkSessionContext() {
  const context = inject(WALK_SESSION_KEY);

  if (!context) {
    throw new Error("walkSession context is not available.");
  }

  return context;
}

export function useAppNavigation() {
  const router = useRouter();
  const walkSession = useWalkSessionContext();

  function go(target, query) {
    const path = LEGACY_ROUTE_MAP[target] || target;
    router.push(query ? { path, query } : path);
  }

  function goInvite(type) {
    router.push(`/invite/${type}`);
  }

  function goWalk(type) {
    router.push(`/walk/${type}`);
  }

  function randomInvite() {
    const type = inviteTypes[Math.floor(Math.random() * inviteTypes.length)];
    goInvite(type);
  }

  function smartInvite(latest) {
    const type = suggestInviteType(latest) || inviteTypes[0];
    goInvite(type);
  }

  async function startWalk(type) {
    resetWalkSeconds();
    await walkSession.startSession(type);
    router.push(`/walk/${type}`);
  }

  async function finishWalk(type) {
    await walkSession.finishSession();
    router.push(finishRouteFor(type));
  }

  function secondaryInviteRoute(type) {
    if (type === "color") {
      return "/invite/light";
    }

    if (type === "local") {
      return "/invite/sound";
    }

    return "/home";
  }

  function finishRouteFor(type) {
    return type === "color" ? "/finish/color" : "/finish";
  }

  function inviteForType(type) {
    return invites[type] || invites.light;
  }

  return {
    go,
    goInvite,
    goWalk,
    randomInvite,
    smartInvite,
    startWalk,
    finishWalk,
    secondaryInviteRoute,
    finishRouteFor,
    inviteForType,
  };
}
