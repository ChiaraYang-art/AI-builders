const INVITES = {
  light: {
    invite: "invite-light",
    walk: "walk-light",
    pause: "pause-light",
    label: "小芽想出门：light walk",
    title: "带我去楼下晒3分钟太阳好嘛？",
    subtitle: "出门晒太阳咯！",
    speech: ["你知道公园二十分钟定律吗？", "出门晒晒太阳，一会我们都能满血复活！"],
  },
  sound: {
    invite: "invite-sound",
    walk: "walk-sound",
    pause: "pause-sound",
    label: "小芽想出门：sound walk",
    title: "今天去听听城市里的声音，好吗？",
    subtitle: "听听真实世界的声音",
    speech: ["我听见了风、脚步和远处的车声。", "城市今天不像房间里那么安静。"],
  },
  color: {
    invite: "invite-color",
    walk: "walk-color",
    pause: "pause-color",
    label: "小芽想出门：color walk",
    title: "我们去周围找 5 种绿色的东西，怎么样？",
    subtitle: "收集今天路上的绿色",
    speech: ["路边的颜色比屏幕里柔软很多。", "我想把这些绿色记进今天的日记。"],
  },
  local: {
    invite: "invite-local",
    walk: "walk-local",
    pause: "pause-local",
    label: "小芽想出门：local discovery",
    title: "国权路的咖啡店换了新菜单，我们走路过去看看路边的银杏吧！",
    subtitle: "看看附近新长出的线索",
    speech: ["附近也有很多新的东西在长出来。", "我们可以慢慢走过去看一看。"],
    local: true,
  },
};

const routeToInvite = {
  "invite-light": "light",
  "invite-sound": "sound",
  "invite-color": "color",
  "invite-local": "local",
  "walk-light": "light",
  "walk-sound": "sound",
  "walk-color": "color",
  "walk-local": "local",
  "pause-light": "light",
  "pause-sound": "sound",
  "pause-color": "color",
  "pause-local": "local",
};

const state = {
  route: "home",
  previousRoute: "home",
  currentInvite: "light",
  seconds: 182,
  timerId: null,
  toast: "",
  shareSheet: false,
};

const app = document.querySelector("#app");

function setRoute(route) {
  state.previousRoute = state.route;
  state.route = route;
  state.shareSheet = false;
  state.toast = "";
  if (routeToInvite[route]) state.currentInvite = routeToInvite[route];
  if (!route.startsWith("walk-")) stopTimer();
  render();
}

function randomInvite() {
  const keys = Object.keys(INVITES);
  const key = keys[Math.floor(Math.random() * keys.length)];
  setRoute(INVITES[key].invite);
}

function startWalk(key) {
  state.currentInvite = key;
  state.seconds = 182;
  setRoute(INVITES[key].walk);
}

function startTimer() {
  stopTimer();
  state.timerId = window.setInterval(() => {
    state.seconds += 1;
    const node = document.querySelector("[data-walk-time]");
    if (node) node.textContent = walkTime();
  }, 1000);
}

function stopTimer() {
  if (state.timerId) {
    window.clearInterval(state.timerId);
    state.timerId = null;
  }
}

function walkTime() {
  const minutes = String(Math.floor(state.seconds / 60)).padStart(2, "0");
  const seconds = String(state.seconds % 60).padStart(2, "0");
  return `已散步时间00：${minutes}：${seconds}`;
}

function icon(name) {
  const paths = {
    home: '<path d="M4 10.5C4 6.9 7 4.5 12 3c5 1.5 8 3.9 8 7.5 0 4.7-3.7 8-8 9.5-4.3-1.5-8-4.8-8-9.5Z"/><path d="M8 9c0 4 4 7 4 7s4-3 4-7"/><path d="M12 16V8"/>',
    walk: '<path d="M12 5a2 2 0 1 0 0-4 2 2 0 0 0 0 4Z"/><path d="M10 8 8 13l-3 6"/><path d="m13 9 3 3 3 1"/><path d="m11 14 4 6"/>',
    diary: '<path d="M6 3h12v18H6z"/><path d="M9 7h6"/><path d="M9 11h6"/><path d="M9 15h4"/>',
    nearby: '<path d="M12 21s7-5.1 7-11a7 7 0 1 0-14 0c0 5.9 7 11 7 11Z"/><circle cx="12" cy="10" r="2.5"/>',
    me: '<circle cx="12" cy="8" r="4"/><path d="M5 21c1.3-4 12.7-4 14 0"/>',
  };
  return `<svg viewBox="0 0 24 24" aria-hidden="true">${paths[name]}</svg>`;
}

function nav(active, options = {}) {
  const homeRoute = options.homeRoute || "home";
  const items = [
    ["home", "首页", homeRoute, null],
    ["walk", "散步", null, "random"],
    ["diary", "日记", "diary", null],
    ["nearby", "附近", "nearby", null],
    ["me", "我的", "me", null],
  ];
  return `
    <nav class="bottom-nav ${options.long ? "" : ""}">
      ${items
        .map(([key, label, route, action]) => `
          <button type="button" class="nav-item ${active === key ? "active" : ""}" ${route ? `data-route="${route}"` : ""} ${action ? `data-action="${action}"` : ""}>
            <span class="nav-icon">${icon(key)}</span>
            ${label}
          </button>
        `)
        .join("")}
    </nav>
  `;
}

function header({ title, subtitle = "", backRoute = "" }) {
  return `
    <p class="time">9:41</p>
    ${backRoute ? `<button type="button" class="back" data-route="${backRoute}" aria-label="返回">‹</button>` : ""}
    <h1 class="title ${backRoute ? "with-back" : ""}">${title}</h1>
    ${subtitle ? `<p class="subtitle ${backRoute ? "with-back" : ""}">${subtitle}</p>` : ""}
  `;
}

function sensors(outdoor = false) {
  const rows = outdoor
    ? ["在室外", "光照充足", "有城市的声音", "温度28.3℃ 湿度43.1%"]
    : ["在室内", "光照偏低", "很安静", "温度28.3℃ 湿度43.1%"];
  return `
    <ul class="sensor">
      ${rows.map((row, index) => `<li>${index === 0 ? `<strong>${row}</strong>` : row}</li>`).join("")}
    </ul>
  `;
}

function sprout(mood = "wilted", extraClass = "") {
  return `
    <div class="sprout ${mood === "happy" ? "happy" : ""} ${extraClass}" aria-label="小芽形象">
      <div class="sprout-body">
        <span class="soil"></span><span class="soil"></span><span class="soil"></span><span class="soil"></span><span class="soil"></span>
        <span class="eye left"></span><span class="eye right"></span>
      </div>
      ${["one", "two", "three"].map((name) => `<span class="flower ${name}"><i class="petal"></i><i class="petal"></i><i class="petal"></i><i class="petal"></i><i class="petal"></i></span>`).join("")}
    </div>
  `;
}

function tinySprout() {
  return `
    <div class="tiny-sprout" aria-hidden="true">
      <span class="leaf-a"></span><span class="leaf-b"></span><span class="stem"></span><span class="pot"></span>
    </div>
  `;
}

function homeScreen() {
  return `
    <section class="screen">
      ${header({ title: "有点蔫了", subtitle: "我们上一次出门是 5 天前" })}
      ${sensors(false)}
      <button type="button" class="speech-card" data-action="random">
        <p>今天这里只有屏幕光......</p>
        <p>可以带我去找一点真实的太阳吗？</p>
      </button>
      ${sprout("wilted")}
      ${nav("home")}
    </section>
  `;
}

function inviteScreen(key) {
  const invite = INVITES[key];
  const secondaryTarget = key === "color" ? "invite-light" : key === "local" ? "invite-sound" : "home";
  return `
    <section class="screen">
      ${header({ title: "出门邀请", subtitle: "一个很小的照顾任务" })}
      ${sensors(false)}
      <article class="prompt-card ${invite.local ? "local" : ""}">
        <p class="prompt-label">${invite.label}</p>
        <h2 class="prompt-title ${invite.local ? "small" : ""}">${invite.title}</h2>
        <div class="button-row">
          <button type="button" class="button primary" data-start-walk="${key}">出门！</button>
          <button type="button" class="button secondary" data-route="${secondaryTarget}">${key === "light" || key === "sound" ? "今天算了吧" : "换个更轻松的任务"}</button>
        </div>
      </article>
      ${sprout("wilted")}
      ${nav("walk")}
    </section>
  `;
}

function walkScreen(key) {
  const invite = INVITES[key];
  window.requestAnimationFrame(startTimer);
  return `
    <section class="screen">
      ${header({ title: key === "light" ? "Light Walk" : key === "sound" ? "Sound Walk" : key === "color" ? "Color Walk" : "Local Discovery", subtitle: invite.subtitle, backRoute: invite.pause })}
      ${sensors(true)}
      <p class="walk-time" data-walk-time>${walkTime()}</p>
      <article class="speech-card">
        <p>${invite.speech[0]}</p>
        <p>${invite.speech[1]}</p>
      </article>
      ${sprout("happy", "walk")}
      ${nav("walk", { homeRoute: invite.pause })}
    </section>
  `;
}

function pauseScreen(key) {
  const invite = INVITES[key];
  const finishRoute = key === "color" ? "finish-color" : "finish";
  return `
    <section class="screen">
      ${header({ title: "散步暂停", subtitle: "可以继续，也可以先回去" })}
      <article class="pause-card">
        <p class="prompt-label">中途退出</p>
        <h2>要先休息一下吗？</h2>
        <p>小芽会保留刚才收集到的光、声音和移动记录。</p>
        <div class="button-row">
          <button type="button" class="button primary" data-route="${invite.walk}">继续散步</button>
          <button type="button" class="button secondary" data-route="${finishRoute}">完成散步</button>
        </div>
      </article>
    </section>
  `;
}

function finishScreen(color = false) {
  return `
    <section class="screen">
      ${header({ title: "散步完成", backRoute: "home" })}
      <div class="complete-hero">${sprout("happy")}</div>
      <h2 class="complete-title">我活过来了!</h2>
      <article class="complete-speech">
        ${color ? "我把今天路上的绿色都记住了。谢谢你带我去看真实的颜色，我觉得自己又长出了一点新叶。" : "刚才的光比房间亮很多，我还听见了风和脚步声。谢谢你带我出去，我觉得自己又长出了一点新叶。"}
      </article>
      <div class="button-row" style="position:absolute;left:36px;top:511px;margin:0;gap:28px;">
        <button type="button" class="button primary wide" data-route="home">返回首页</button>
        <button type="button" class="button secondary wide" data-route="${color ? "diary-log-color" : "diary-log"}">查看散步日记</button>
      </div>
      <article class="saved-card">
        <h3>已保存</h3>
        <p>这次散步会出现在小芽日记里。</p>
      </article>
      ${nav("diary")}
    </section>
  `;
}

function diaryScreen() {
  const done = new Set([3, 7, 10, 14, 18, 21, 24]);
  return `
    <section class="screen">
      ${header({ title: "小芽日记", subtitle: "每天的小天气，都会长成记忆" })}
      <article class="content-card calendar-card" data-route="diary-log">
        <div class="calendar-head"><span>‹</span><strong>May 2026</strong><span>›</span></div>
        <p class="calendar-summary">12 walks · 5 sunny days · 3 new sounds</p>
        <div class="achievement-chip">成就：城市探索家</div>
        <div class="calendar-grid">
          ${["M", "T", "W", "T", "F", "S", "S"].map((d) => `<span class="weekday">${d}</span>`).join("")}
          <span></span><span></span><span></span>
          ${Array.from({ length: 31 }, (_, i) => {
            const day = i + 1;
            const cls = day === 28 ? "day today" : day === 10 ? "day sound" : day === 14 || day === 21 ? "day sunny" : done.has(day) ? "day done" : "day";
            return `<span class="${cls}">${day}</span>`;
          }).join("")}
        </div>
      </article>
      <article class="content-card map-card" data-route="map">
        <h2>散步地图</h2>
        <p>在地图上查看这个月和小芽一起留下的散步记忆</p>
        <div class="mini-map"></div>
      </article>
      ${nav("diary")}
    </section>
  `;
}

function diaryLogScreen(color = false) {
  return `
    <section class="screen">
      ${header({ title: "小芽日记", subtitle: color ? "Color Walk 记录" : "2026 年 5 月", backRoute: "diary" })}
      <div class="tabs">
        <button type="button" class="active">流水账</button>
        <button type="button" data-route="${color ? "diary-essay-color" : "diary-essay"}">小作文</button>
      </div>
      <article class="essay-card">
        <h2>${color ? "绿色收集记录" : "今天的小天气"}</h2>
        <p>${color ? "按顺序记录路上遇到的颜色。" : "按时间记录传感器看到的变化。"}</p>
        <div class="log-list">
          <div class="log-item"><strong>15:02 光照变亮</strong><p>lux 从 86 上升到 520，小芽判断我们走到室外。</p></div>
          <div class="log-item"><strong>15:08 听见城市声音</strong><p>声音波动变大，可能经过了路口和树下。</p></div>
          <div class="log-item"><strong>15:18 ${color ? "拍到 5 种绿色" : "小芽恢复精神"}</strong><p>${color ? "银杏、草丛、店门口的小盆栽、路牌和树影。" : "这次散步会被保存进小芽日记。"}</p></div>
        </div>
      </article>
      ${nav("diary")}
    </section>
  `;
}

function diaryEssayScreen(color = false) {
  if (color) {
    return `
      <section class="screen scroll">
        <div class="long-page">
          ${header({ title: "小芽日记", subtitle: "Color Walk 小作文", backRoute: "diary" })}
          <div class="tabs">
            <button type="button" data-route="diary-log">流水账</button>
            <button type="button" class="active">小作文</button>
          </div>
          <article class="essay-card" style="min-height:860px;">
            <h2>今天我收集了很多绿色</h2>
            <p>我们从房间里出发，先遇见了树影，然后遇见了草丛、路牌和店门口的小盆栽。</p>
            <p>这些绿色不是一种颜色。它们有的很亮，有的偏黄，有的藏在阴影里。小芽把它们都记住了。</p>
            <p>如果以后我又有点蔫了，可以翻回这一天，看见你带我去过真实的街道。</p>
            <div class="button-row" style="position:absolute;left:181px;top:914px;margin:0;">
              <button type="button" class="button primary" data-route="share">分享</button>
            </div>
          </article>
          ${nav("diary", { long: true })}
        </div>
      </section>
    `;
  }
  return `
    <section class="screen">
      ${header({ title: "小芽日记", subtitle: "今日散步小作文", backRoute: "diary" })}
      <div class="tabs">
        <button type="button" data-route="diary-log">流水账</button>
        <button type="button" class="active">小作文</button>
      </div>
      <article class="essay-card">
        <h2>今天我听见了春天</h2>
        <p>今天我们走到了有树影的路边。阳光先落在我的叶子上，然后慢慢变暖。</p>
        <p>我听见了车轮、脚步和风。你拍下的绿色像新的叶尖，提醒我真实世界一直在悄悄长大。</p>
        <div class="button-row" style="margin-top:24px;">
          <button type="button" class="button primary" data-route="share">分享</button>
        </div>
      </article>
      ${nav("diary")}
    </section>
  `;
}

function mapScreen() {
  return `
    <section class="screen">
      ${header({ title: "散步地图", subtitle: "这个月和小芽走过的地方" })}
      <article class="essay-card" style="top:132px;">
        <h2>国权路 · 树影路线</h2>
        <p>32 min walk · 3 sounds · 5 greens</p>
        <div class="mini-map" style="position:relative;left:auto;bottom:auto;width:305px;height:260px;margin-top:22px;border-radius:24px;"></div>
      </article>
      ${nav("diary")}
    </section>
  `;
}

function nearbyScreen() {
  return `
    <section class="screen">
      ${header({ title: "附近", subtitle: "城市里新长出来的线索" })}
      <article class="prompt-card local">
        <p class="prompt-label">Local Discovery</p>
        <h2 class="prompt-title small">国权路的咖啡店换了新菜单。如果我们走路过去，还能看看路边的银杏。</h2>
      </article>
      ${sprout("wilted")}
      ${nav("nearby")}
    </section>
  `;
}

function meScreen() {
  return `
    <section class="screen">
      ${header({ title: "我的" })}
      <article class="profile-card">
        ${sprout("happy", "small-profile")}
        <div class="profile-text">
          <h2>Chiara的小芽</h2>
          <p>成长 18 天 · 今日很想见光</p>
          <p>已一起散步7次</p>
          <p>成长值：65</p>
          <div class="growth"><span></span></div>
        </div>
      </article>
      <article class="settings-card">
        ${["设备状态", "语音音色", "提醒时间", "数据权限"].map((row) => `<div class="setting-row"><span>${row}</span><span>›</span></div>`).join("")}
      </article>
      <article class="atlas-entry" data-route="atlas">
        <h2>我解锁的小芽 <span style="float:right;color:#9aa08e;">›</span></h2>
        <div class="badges"><span class="badge"></span><span class="badge"></span><span class="badge"></span><span class="badge"></span></div>
      </article>
      ${nav("me")}
    </section>
  `;
}

function atlasScreen() {
  const items = [
    ["Light Sprout", "已解锁", false],
    ["Color Sprout", "已解锁", false],
    ["City Sprout", "已解锁", false],
    ["Sound Sprout", "已解锁", false],
    ["Night Sprout", "去散步解锁", true],
    ["Bloom Sprout", "去散步解锁", true],
  ];
  return `
    <section class="screen">
      ${header({ title: "小芽图鉴", subtitle: "收集不同的小芽形态", backRoute: "me" })}
      <div class="atlas-grid">
        ${items.map(([name, status, locked]) => `<article class="atlas-card ${locked ? "locked" : ""}">${tinySprout()}<h3>${name}</h3><p>${status}</p></article>`).join("")}
      </div>
      ${nav("me")}
    </section>
  `;
}

function shareScreen() {
  return `
    <section class="screen">
      ${header({ title: "分享卡片", subtitle: "把今天的小芽日记发给朋友", backRoute: "diary-essay" })}
      <article class="share-card">
        <div class="share-hero">${sprout("happy", "share-mini")}<span class="share-sun"></span><span class="share-tree"></span></div>
        <h2>今天我听见了春天</h2>
        <p>32 min walk · 3 sounds · 5 greens</p>
        <p style="margin-top:18px;">刚才的光不是屏幕里的光。它落在我的叶子上，也落在你的鞋尖前面。</p>
        <span class="qr" aria-hidden="true"></span>
      </article>
      <div class="button-row" style="position:absolute;left:36px;top:632px;margin:0;gap:28px;">
        <button type="button" class="button primary wide" data-action="save-share">保存图片</button>
        <button type="button" class="button secondary wide" data-action="open-share">分享</button>
      </div>
      <article class="privacy-card"><h3>隐私提示</h3><p>分享卡片默认不包含精确位置。</p></article>
      ${state.toast ? `<div class="toast">${state.toast}</div>` : ""}
      ${state.shareSheet ? `<div class="share-sheet"><h3>分享到</h3><div class="share-options"><button>小红书</button><button>微信</button><button>朋友圈</button></div></div>` : ""}
    </section>
  `;
}

function render() {
  const routes = {
    home: homeScreen,
    "invite-light": () => inviteScreen("light"),
    "invite-sound": () => inviteScreen("sound"),
    "invite-color": () => inviteScreen("color"),
    "invite-local": () => inviteScreen("local"),
    "walk-light": () => walkScreen("light"),
    "walk-sound": () => walkScreen("sound"),
    "walk-color": () => walkScreen("color"),
    "walk-local": () => walkScreen("local"),
    "pause-light": () => pauseScreen("light"),
    "pause-sound": () => pauseScreen("sound"),
    "pause-color": () => pauseScreen("color"),
    "pause-local": () => pauseScreen("local"),
    finish: () => finishScreen(false),
    "finish-color": () => finishScreen(true),
    diary: diaryScreen,
    "diary-log": () => diaryLogScreen(false),
    "diary-log-color": () => diaryLogScreen(true),
    "diary-essay": () => diaryEssayScreen(false),
    "diary-essay-color": () => diaryEssayScreen(true),
    map: mapScreen,
    nearby: nearbyScreen,
    me: meScreen,
    atlas: atlasScreen,
    share: shareScreen,
  };
  app.innerHTML = (routes[state.route] || homeScreen)();
}

document.addEventListener("click", (event) => {
  const action = event.target.closest("[data-action]");
  const route = event.target.closest("[data-route]");
  const start = event.target.closest("[data-start-walk]");
  const jump = event.target.closest("[data-jump]");

  if (action) {
    const name = action.dataset.action;
    if (name === "random") randomInvite();
    if (name === "save-share") {
      state.toast = "图片已保存";
      state.shareSheet = false;
      render();
    }
    if (name === "open-share") {
      state.shareSheet = true;
      state.toast = "";
      render();
    }
    return;
  }

  if (start) {
    startWalk(start.dataset.startWalk);
    return;
  }

  if (route) {
    setRoute(route.dataset.route);
    return;
  }

  if (jump) {
    setRoute(jump.dataset.jump);
  }
});

render();
