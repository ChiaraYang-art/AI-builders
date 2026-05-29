const screens = {
  home: "https://www.figma.com/api/mcp/asset/bffb99c5-61a1-4356-8324-91f8693552be",
  inviteLight: "https://www.figma.com/api/mcp/asset/dd5f1e3c-6231-4a7e-bcf3-11bfdd74d7b2",
  inviteSound: "https://www.figma.com/api/mcp/asset/85ea4298-dd5d-49e7-8ff0-9b2ec57ff2ff",
  inviteColor: "https://www.figma.com/api/mcp/asset/876e5b9b-57fd-4286-8004-4478edd7fda4",
  inviteLocal: "https://www.figma.com/api/mcp/asset/a4ee2477-7259-4fd2-a4bf-8bd9179251a6",
  walkLight: "https://www.figma.com/api/mcp/asset/618dd3ac-cc0f-4da7-af2c-cd24e2fb6abf",
  walkSound: "https://www.figma.com/api/mcp/asset/93e1a957-6912-4084-a945-9ff7c3a939ff",
  walkColor1: "https://www.figma.com/api/mcp/asset/a27949da-4a2c-436a-a82a-ddb7e77364db",
  walkColor2: "https://www.figma.com/api/mcp/asset/4d4a26ec-7e43-48b5-af5b-e43d8bfb1013",
  pauseLight: "https://www.figma.com/api/mcp/asset/abc83246-8374-40ab-bb1c-17a080478c69",
  pauseSound: "https://www.figma.com/api/mcp/asset/69216858-c254-48f0-ab90-d65ba6c518ee",
  finish: "https://www.figma.com/api/mcp/asset/fd39bcb2-7728-4193-a225-5bdc364f250e",
  diary: "https://www.figma.com/api/mcp/asset/d1df4bda-ae49-4289-840f-4d5331ee3948",
  diaryLog: "https://www.figma.com/api/mcp/asset/acc29fac-b315-4bd2-95ec-168452b29a24",
  diaryEssay: "https://www.figma.com/api/mcp/asset/432a7b5c-ef22-4df2-a908-15a674e5c506",
  share: "https://www.figma.com/api/mcp/asset/3e0167ea-09ff-4673-99e4-208c263e6c9b",
  map: "https://www.figma.com/api/mcp/asset/bcdc11c2-a10a-49fc-8234-9519e27f6885",
  nearby: "https://www.figma.com/api/mcp/asset/336fd2cd-94ad-422c-8a0d-0ec2d01cbd73",
  atlas: "https://www.figma.com/api/mcp/asset/be36f02f-7a82-4379-8935-b2fee74f026a",
  me: "https://www.figma.com/api/mcp/asset/2541923b-d02c-42ac-9b6c-5e344aa2b8a1",
};

const image = document.querySelector("#screen-image");
const hotspotLayer = document.querySelector("#hotspots");

let current = "home";

const inviteTargets = ["inviteLight", "inviteSound", "inviteColor", "inviteLocal"];

function randomInvite() {
  const index = Math.floor(Math.random() * inviteTargets.length);
  return inviteTargets[index];
}

function go(target) {
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

function addNav(active) {
  addHotspot({ x: 38, y: 770, w: 42, h: 62, target: "home", label: "首页" });
  addHotspot({ x: 102, y: 770, w: 52, h: 62, target: active === "walk" ? current : "inviteLight", label: "散步" });
  addHotspot({ x: 170, y: 770, w: 52, h: 62, target: "diary", label: "日记" });
  addHotspot({ x: 240, y: 770, w: 52, h: 62, target: "nearby", label: "附近" });
  addHotspot({ x: 310, y: 770, w: 52, h: 62, target: "me", label: "我的" });
}

function renderHotspots() {
  hotspotLayer.innerHTML = "";

  if (current === "home") {
    addHotspot({ x: 51, y: 344, w: 292, h: 104, action: "randomInvite", label: "小芽说话框，随机出门邀请" });
    addNav("home");
    return;
  }

  if (current === "inviteLight") {
    addHotspot({ x: 51, y: 373, w: 142, h: 44, target: "walkLight", label: "出门" });
    addHotspot({ x: 200, y: 373, w: 142, h: 44, target: "home", label: "今天算了吧" });
    addNav("walk");
    return;
  }

  if (current === "inviteSound") {
    addHotspot({ x: 51, y: 373, w: 142, h: 44, target: "walkSound", label: "出门" });
    addHotspot({ x: 200, y: 373, w: 142, h: 44, target: "home", label: "今天算了吧" });
    addNav("walk");
    return;
  }

  if (current === "inviteColor") {
    addHotspot({ x: 53, y: 373, w: 142, h: 44, target: "walkColor1", label: "出门" });
    addHotspot({ x: 200, y: 373, w: 142, h: 44, target: "inviteLight", label: "换个更轻松的任务" });
    addNav("walk");
    return;
  }

  if (current === "inviteLocal") {
    addHotspot({ x: 51, y: 376, w: 142, h: 44, target: "walkColor2", label: "出门" });
    addHotspot({ x: 200, y: 376, w: 142, h: 44, target: "inviteSound", label: "换个更轻松的任务" });
    addNav("walk");
    return;
  }

  if (current === "walkLight") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "pauseLight", label: "暂停散步" });
    addNav("walk");
    return;
  }

  if (current === "walkSound") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "pauseSound", label: "暂停散步" });
    addNav("walk");
    return;
  }

  if (current === "walkColor1" || current === "walkColor2") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "finish", label: "结束散步" });
    addNav("walk");
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

  if (current === "finish") {
    addHotspot({ x: 199, y: 714, w: 90, h: 40, target: "share", label: "分享" });
    addNav("diary");
    return;
  }

  if (current === "diary") {
    addHotspot({ x: 20, y: 132, w: 353, h: 414, target: "diaryLog", label: "日历" });
    addHotspot({ x: 20, y: 563, w: 353, h: 179, target: "map", label: "散步地图" });
    addNav("diary");
    return;
  }

  if (current === "diaryLog") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "diary", label: "返回" });
    addHotspot({ x: 247, y: 130, w: 90, h: 48, target: "diaryEssay", label: "小作文" });
    addNav("diary");
    return;
  }

  if (current === "diaryEssay") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "diary", label: "返回" });
    addHotspot({ x: 199, y: 714, w: 90, h: 40, target: "share", label: "分享" });
    addHotspot({ x: 80, y: 130, w: 90, h: 48, target: "diaryLog", label: "流水账" });
    addNav("diary");
    return;
  }

  if (current === "share") {
    addHotspot({ x: 24, y: 58, w: 34, h: 34, target: "diaryEssay", label: "返回" });
    return;
  }

  if (current === "map") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "diary", label: "返回" });
    addNav("diary");
    return;
  }

  if (current === "nearby") {
    addNav("nearby");
    return;
  }

  if (current === "me") {
    addHotspot({ x: 20, y: 559, w: 353, h: 156, target: "atlas", label: "小芽图鉴入口" });
    addNav("me");
    return;
  }

  if (current === "atlas") {
    addHotspot({ x: 23, y: 57, w: 34, h: 34, target: "me", label: "返回" });
    addNav("me");
  }
}

document.querySelectorAll("[data-go]").forEach((button) => {
  button.addEventListener("click", () => go(button.dataset.go));
});

go("home");
