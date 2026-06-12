const screens = {
  home: "./figma_dev_assets/home.png",
  inviteColor: "./figma_dev_assets/inviteColor.png",
  inviteLight: "./figma_dev_assets/inviteLight.png",
  inviteSound: "./figma_dev_assets/inviteSound.png",
  inviteLocal: "./figma_dev_assets/inviteLocal.png",
  walkLight: "./figma_dev_assets/walkLight.png",
  walkColor: "./figma_dev_assets/walkColor1.png",
  walkLocal: "./figma_dev_assets/walkColor2.png",
  walkSound: "./figma_dev_assets/walkSound.png",
  pauseLight: "./figma_dev_assets/pauseLight.png",
  pauseSound: "./figma_dev_assets/pauseSound.png",
  pauseColor: "./figma_dev_assets/pauseColor.png",
  pauseLocal: "./figma_dev_assets/pauseLocal.png",
  finish: "./figma_dev_assets/finish.png",
  finishColor: "./figma_dev_assets/finishColor.png",
  diary: "./figma_dev_assets/diary.png",
  diaryLog: "./figma_dev_assets/diaryLog.png",
  diaryLogColor: "./figma_dev_assets/diaryLogColor.png",
  diaryEssay: "./figma_dev_assets/diaryEssay.png",
  diaryEssayColor: "./figma_dev_assets/diaryEssayColor.png",
  share: "./figma_dev_assets/share.png",
  map: "./figma_dev_assets/map.png",
  nearby: "./figma_dev_assets/nearby.png",
  atlas: "./figma_dev_assets/atlas.png",
  me: "./figma_dev_assets/me.png",
};

const inviteTargets = ["inviteLight", "inviteSound", "inviteColor", "inviteLocal"];
const image = document.querySelector("#screen-image");
const hotspotLayer = document.querySelector("#hotspots");

let current = "home";

function randomInvite() {
  return inviteTargets[Math.floor(Math.random() * inviteTargets.length)];
}

function go(target) {
  if (!screens[target]) return;
  current = target;
  image.src = screens[target];
  image.alt = target;
  renderHotspots();
}

function addHotspot({ x, y, w, h, target, action, label }) {
  const button = document.createElement("button");
  button.type = "button";
  button.className = "hotspot";
  button.style.left = `${x}px`;
  button.style.top = `${y}px`;
  button.style.width = `${w}px`;
  button.style.height = `${h}px`;
  button.setAttribute("aria-label", label || target || action || "hotspot");
  button.addEventListener("click", () => {
    if (action === "randomInvite") go(randomInvite());
    else if (target) go(target);
  });
  hotspotLayer.appendChild(button);
}

function addGenericNav({ active = "", homeTarget = "home", includeHome = true } = {}) {
  if (includeHome) addHotspot({ x: 38, y: 770, w: 42, h: 62, target: homeTarget, label: "首页" });
  addHotspot({ x: 102, y: 770, w: 52, h: 62, action: "randomInvite", label: "散步，随机进入邀请" });
  addHotspot({ x: 170, y: 770, w: 52, h: 62, target: "diary", label: "日记" });
  addHotspot({ x: 240, y: 770, w: 52, h: 62, target: "nearby", label: "附近" });
  addHotspot({ x: 310, y: 770, w: 52, h: 62, target: "me", label: "我的" });
}

function renderHotspots() {
  hotspotLayer.innerHTML = "";

  if (current === "home") {
    addHotspot({ x: 51, y: 344, w: 292, h: 104, action: "randomInvite", label: "首页小芽说话框，随机出门邀请" });
    addGenericNav({ includeHome: false });
    return;
  }

  if (current === "inviteLight") {
    addHotspot({ x: 51, y: 373, w: 142, h: 44, target: "walkLight", label: "出门" });
    addGenericNav();
    return;
  }

  if (current === "inviteSound") {
    addHotspot({ x: 51, y: 373, w: 142, h: 44, target: "walkSound", label: "出门" });
    addGenericNav();
    return;
  }

  if (current === "inviteColor") {
    addHotspot({ x: 53, y: 373, w: 142, h: 44, target: "walkColor", label: "出门" });
    addHotspot({ x: 200, y: 373, w: 142, h: 44, target: "inviteLight", label: "换个更轻松的任务" });
    addGenericNav();
    return;
  }

  if (current === "inviteLocal") {
    addHotspot({ x: 51, y: 376, w: 142, h: 44, target: "walkLocal", label: "出门" });
    addHotspot({ x: 200, y: 376, w: 142, h: 44, target: "inviteSound", label: "换个更轻松的任务" });
    addGenericNav();
    return;
  }

  if (current === "walkLight") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "pauseLight", label: "暂停散步" });
    addGenericNav({ homeTarget: "pauseLight" });
    return;
  }

  if (current === "walkSound") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "pauseSound", label: "暂停散步" });
    addGenericNav({ homeTarget: "pauseSound" });
    return;
  }

  if (current === "walkColor") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "pauseColor", label: "暂停散步" });
    addGenericNav({ homeTarget: "pauseColor" });
    return;
  }

  if (current === "walkLocal") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "pauseLocal", label: "暂停散步" });
    addGenericNav({ homeTarget: "pauseLocal" });
    return;
  }

  if (current === "pauseLight") {
    addHotspot({ x: 48, y: 461, w: 132, h: 50, target: "walkLight", label: "继续散步" });
    addHotspot({ x: 207, y: 461, w: 126, h: 50, target: "finish", label: "完成散步" });
    return;
  }

  if (current === "pauseSound") {
    addHotspot({ x: 48, y: 461, w: 132, h: 50, target: "walkSound", label: "继续散步" });
    addHotspot({ x: 207, y: 461, w: 126, h: 50, target: "finish", label: "完成散步" });
    return;
  }

  if (current === "pauseColor") {
    addHotspot({ x: 48, y: 461, w: 132, h: 50, target: "walkColor", label: "继续散步" });
    addHotspot({ x: 207, y: 461, w: 126, h: 50, target: "finishColor", label: "完成散步" });
    return;
  }

  if (current === "pauseLocal") {
    addHotspot({ x: 48, y: 461, w: 132, h: 50, target: "walkLocal", label: "继续散步" });
    addHotspot({ x: 207, y: 461, w: 126, h: 50, target: "finish", label: "完成散步" });
    return;
  }

  if (current === "finish") {
    addHotspot({ x: 210, y: 511, w: 146, h: 50, target: "diaryLog", label: "查看散步日记" });
    addGenericNav({ active: "diary" });
    return;
  }

  if (current === "finishColor") {
    addHotspot({ x: 210, y: 511, w: 146, h: 50, target: "diaryLogColor", label: "查看散步日记" });
    addGenericNav({ active: "diary" });
    return;
  }

  if (current === "diary") {
    addHotspot({ x: 20, y: 132, w: 353, h: 414, target: "diaryLog", label: "日历" });
    addHotspot({ x: 20, y: 563, w: 353, h: 179, target: "map", label: "散步地图" });
    addGenericNav({ includeHome: true });
    return;
  }

  if (current === "diaryLog") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "diary", label: "返回" });
    addHotspot({ x: 247, y: 130, w: 90, h: 48, target: "diaryEssay", label: "小作文" });
    addGenericNav();
    return;
  }

  if (current === "diaryLogColor") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "diary", label: "返回" });
    addHotspot({ x: 247, y: 130, w: 90, h: 48, target: "diaryEssayColor", label: "小作文" });
    addGenericNav();
    return;
  }

  if (current === "diaryEssay") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "diary", label: "返回" });
    addHotspot({ x: 199, y: 714, w: 90, h: 40, target: "share", label: "分享" });
    addHotspot({ x: 80, y: 130, w: 90, h: 48, target: "diaryLog", label: "流水账" });
    addGenericNav();
    return;
  }

  if (current === "diaryEssayColor") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "diary", label: "返回" });
    addHotspot({ x: 80, y: 130, w: 90, h: 48, target: "diaryLog", label: "流水账" });
    addGenericNav();
    return;
  }

  if (current === "share") {
    addHotspot({ x: 24, y: 58, w: 34, h: 34, target: "diaryEssay", label: "返回" });
    return;
  }

  if (current === "map") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "diary", label: "返回" });
    addGenericNav();
    return;
  }

  if (current === "nearby") {
    addGenericNav({ active: "nearby" });
    return;
  }

  if (current === "me") {
    addHotspot({ x: 20, y: 559, w: 353, h: 156, target: "atlas", label: "小芽图鉴入口" });
    addGenericNav({ active: "me" });
    return;
  }

  if (current === "atlas") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "me", label: "返回" });
    addGenericNav({ active: "me" });
  }
}

document.addEventListener("click", (event) => {
  const goButton = event.target.closest("[data-go]");
  const randomButton = event.target.closest("[data-random-invite]");

  if (randomButton) {
    go(randomInvite());
    return;
  }

  if (goButton) {
    go(goButton.dataset.go);
  }
});

go("home");
