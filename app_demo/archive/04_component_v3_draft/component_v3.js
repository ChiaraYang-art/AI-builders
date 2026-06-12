const INVITES = {
  light: {
    screen: "invite-light",
    walk: "walk-light",
    label: "小芽想出门：light walk",
    title: "带我去楼下晒3分钟太阳好嘛？",
    cancel: "今天算了吧",
    speech: "你知道公园二十分钟定律吗？出门晒晒太阳，一会我们都能满血复活！",
    walkTitle: "Light Walk",
    walkSubtitle: "出门晒太阳咯！",
  },
  sound: {
    screen: "invite-sound",
    walk: "walk-sound",
    label: "小芽想出门：sound walk",
    title: "今天去听听城市里的声音，好吗？",
    cancel: "今天算了吧",
    speech: "我听见了风、脚步和远处的车声。",
    walkTitle: "Sound Walk",
    walkSubtitle: "听听真实世界的声音",
  },
  color: {
    screen: "invite-color",
    walk: "walk-color",
    label: "小芽想出门：color walk",
    title: "我们去周围找 5 种绿色的东西，怎么样？",
    cancel: "换个更轻松的任务",
    speech: "路边的颜色比屏幕里柔软很多。",
    walkTitle: "Color Walk",
    walkSubtitle: "收集今天路上的绿色",
  },
  local: {
    screen: "invite-local",
    walk: "walk-local",
    label: "小芽想出门：local discovery",
    title: "国权路的咖啡店换了新菜单，我们走路过去看看路边的银杏吧！",
    cancel: "换个更轻松的任务",
    speech: "附近也有很多新的东西在长出来。",
    walkTitle: "Local Discovery",
    walkSubtitle: "看看附近新长出的线索",
    local: true,
  },
};

const state = {
  route: "home",
  previousRoute: "home",
  currentInvite: "light",
  seconds: 182,
  timerId: null,
};

const app = document.querySelector("#app");

function setRoute(route) {
  state.previousRoute = state.route;
  state.route = route;

  if (!route.startsWith("walk-")) {
    stopTimer();
  }

  render();
}

function randomInvite() {
  const keys = Object.keys(INVITES);
  state.currentInvite = keys[Math.floor(Math.random() * keys.length)];
  setRoute(INVITES[state.currentInvite].screen);
}

function startWalk(inviteKey = state.currentInvite) {
  state.currentInvite = inviteKey;
  state.seconds = 182;
  setRoute(INVITES[inviteKey].walk);
}

function startTimer() {
  stopTimer();
  state.timerId = window.setInterval(() => {
    state.seconds += 1;
    const timer = document.querySelector("[data-walk-time]");
    if (timer) timer.textContent = formatWalkTime(state.seconds);
  }, 1000);
}

function stopTimer() {
  if (state.timerId) {
    window.clearInterval(state.timerId);
    state.timerId = null;
  }
}

function formatWalkTime(totalSeconds) {
  const minutes = String(Math.floor(totalSeconds / 60)).padStart(2, "0");
  const seconds = String(totalSeconds % 60).padStart(2, "0");
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

  return `<svg class="nav-svg" viewBox="0 0 24 24" aria-hidden="true">${paths[name]}</svg>`;
}

function nav(active) {
  const items = [
    ["home", "首页", "home"],
    ["walk", "散步", "invite-light"],
    ["diary", "日记", "diary"],
    ["nearby", "附近", "nearby"],
    ["me", "我的", "me"],
  ];

  return `
    <nav class="nav" aria-label="底部导航">
      ${items
        .map(
          ([key, label, route]) => `
            <button type="button" class="${active === key ? "active" : ""}" data-go="${route}">
              <span class="nav-icon">${icon(key)}</span>
              ${label}
            </button>
          `
        )
        .join("")}
    </nav>
  `;
}

function header({ title, subtitle, back = false }) {
  return `
    <p class="time">9:41</p>
    ${back ? '<button type="button" class="back-button" data-back aria-label="返回">‹</button>' : ""}
    <h1 class="page-title ${back ? "with-back" : ""}">${title}</h1>
    ${subtitle ? `<p class="page-subtitle ${back ? "with-back" : ""}">${subtitle}</p>` : ""}
  `;
}

function sensors(outdoor = false) {
  const rows = outdoor
    ? ["在室外", "光照充足", "有城市的声音", "温度28.3℃ 湿度43.1%"]
    : ["在室内", "光照偏低", "很安静", "温度28.3℃ 湿度43.1%"];

  return `
    <ul class="sensor-list">
      ${rows
        .map((row, index) => `<li>${index === 0 ? `<strong>${row}</strong>` : row}</li>`)
        .join("")}
    </ul>
  `;
}

function sprout(mood = "wilted") {
  return `
    <div class="mini-sprout ${mood === "happy" ? "happy" : ""}" aria-label="小芽形象">
      <div class="sprout-body">
        <span class="soil"></span>
        <span class="soil"></span>
        <span class="soil"></span>
        <span class="soil"></span>
        <span class="soil"></span>
        <span class="eye left"></span>
        <span class="eye right"></span>
      </div>
      <span class="flower one"><i class="petal"></i><i class="petal"></i><i class="petal"></i><i class="petal"></i><i class="petal"></i></span>
      <span class="flower two"><i class="petal"></i><i class="petal"></i><i class="petal"></i><i class="petal"></i><i class="petal"></i></span>
      <span class="flower three"><i class="petal"></i><i class="petal"></i><i class="petal"></i><i class="petal"></i><i class="petal"></i></span>
    </div>
  `;
}

function tinySprout() {
  return `
    <div class="tiny-sprout" aria-hidden="true">
      <span class="leaf-a"></span>
      <span class="leaf-b"></span>
      <span class="stem"></span>
      <span class="pot"></span>
    </div>
  `;
}

function homeScreen() {
  return `
    <section class="screen">
      ${header({ title: "有点蔫了", subtitle: "我们上一次出门是 5 天前" })}
      ${sensors(false)}
      <button type="button" class="speech-card" data-random-invite>
        <p>今天这里只有屏幕光......</p>
        <p>可以带我去找一点真实的太阳吗？</p>
      </button>
      ${sprout("wilted")}
      ${nav("home")}
    </section>
  `;
}

function inviteScreen(inviteKey) {
  const invite = INVITES[inviteKey];
  return `
    <section class="screen">
      ${header({ title: "出门邀请", subtitle: "一个很小的照顾任务" })}
      ${sensors(false)}
      <article class="prompt-card ${invite.local ? "local" : ""}">
        <p class="prompt-label">${invite.label}</p>
        <h2 class="prompt-title ${invite.local ? "small" : ""}">${invite.title}</h2>
        <div class="button-row">
          <button type="button" class="button primary" data-start-walk="${inviteKey}">出门！</button>
          <button type="button" class="button secondary" data-lighter="${inviteKey}">${invite.cancel}</button>
        </div>
      </article>
      ${sprout("wilted")}
      ${nav("walk")}
    </section>
  `;
}

function walkScreen(inviteKey) {
  const invite = INVITES[inviteKey];
  window.requestAnimationFrame(startTimer);

  return `
    <section class="screen">
      ${header({ title: invite.walkTitle, subtitle: invite.walkSubtitle, back: true })}
      ${sensors(true)}
      <p class="walk-time" data-walk-time>${formatWalkTime(state.seconds)}</p>
      <article class="speech-card">
        <p>${invite.speech.slice(0, 18)}</p>
        <p>${invite.speech.slice(18)}</p>
      </article>
      ${sprout("happy")}
      ${nav("walk")}
    </section>
  `;
}

function pauseScreen() {
  return `
    <section class="screen">
      ${header({ title: "散步暂停", subtitle: "可以继续，也可以先回去", back: true })}
      ${sensors(true)}
      <article class="prompt-card">
        <p class="prompt-label">中途退出</p>
        <h2 class="prompt-title">要先休息一下吗？</h2>
        <div class="button-row">
          <button type="button" class="button primary" data-go="${INVITES[state.currentInvite].walk}">继续</button>
          <button type="button" class="button secondary" data-go="home">退出</button>
        </div>
      </article>
      ${sprout("happy")}
      ${nav("walk")}
    </section>
  `;
}

function finishScreen() {
  return `
    <section class="screen">
      ${header({ title: "散步完成", back: true })}
      <div class="complete-hero">${sprout("happy")}</div>
      <h2 class="complete-title">我活过来了!</h2>
      <article class="complete-speech">
        刚才的光比房间亮很多，我还听见了风和脚步声。谢谢你带我出去，我觉得自己又长出了一点新叶。
      </article>
      <div class="button-row" style="position:absolute;left:36px;top:511px;margin:0;gap:28px;">
        <button type="button" class="button primary wide" data-go="home">返回首页</button>
        <button type="button" class="button secondary wide" data-go="diary-essay">查看散步日记</button>
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
  const days = Array.from({ length: 31 }, (_, i) => i + 1);
  const doneDays = new Set([3, 7, 10, 14, 18, 21, 24]);

  return `
    <section class="screen">
      ${header({ title: "小芽日记", subtitle: "每天的小天气，都会长成记忆" })}
      <article class="content-card calendar-card" data-go="diary-log">
        <div class="calendar-head">
          <button type="button" aria-label="上个月">‹</button>
          <span>May 2026</span>
          <button type="button" aria-label="下个月">›</button>
        </div>
        <p class="calendar-summary">12 walks · 5 sunny days · 3 new sounds</p>
        <div class="achievement-chip">成就：城市探索家</div>
        <div class="calendar-grid">
          ${["M", "T", "W", "T", "F", "S", "S"].map((d) => `<span class="weekday">${d}</span>`).join("")}
          ${"<span></span><span></span><span></span>".repeat(1)}
          ${days
            .map((day) => {
              const className = day === 28 ? "day today" : doneDays.has(day) ? "day done" : day === 14 ? "day sunny" : "day";
              return `<span class="${className}">${day}</span>`;
            })
            .join("")}
        </div>
      </article>
      <article class="content-card map-card" data-go="map">
        <h2>散步地图</h2>
        <p>在地图上查看这个月和小芽一起留下的散步记忆</p>
        <div class="mini-map"></div>
      </article>
      ${nav("diary")}
    </section>
  `;
}

function diaryLogScreen() {
  return `
    <section class="screen">
      ${header({ title: "小芽日记", subtitle: "2026 年 5 月", back: true })}
      <div class="tabs">
        <button type="button" class="active">流水账</button>
        <button type="button" data-go="diary-essay">小作文</button>
      </div>
      <article class="diary-entry with-tabs">
        <h2>今天的小天气</h2>
        <p>按时间记录传感器看到的变化。</p>
        <div class="log-list">
          <div class="log-item"><strong>15:02 光照变亮</strong><p>lux 从 86 上升到 520，小芽判断我们走到室外。</p></div>
          <div class="log-item"><strong>15:08 听见城市声音</strong><p>声音波动变大，可能经过了路口和树下。</p></div>
          <div class="log-item"><strong>15:18 拍到 5 种绿色</strong><p>银杏、草丛、店门口的小盆栽、路牌和树影。</p></div>
        </div>
      </article>
      ${nav("diary")}
    </section>
  `;
}

function diaryEssayScreen() {
  return `
    <section class="screen">
      ${header({ title: "小芽日记", subtitle: "今日散步小作文", back: true })}
      <div class="tabs">
        <button type="button" data-go="diary-log">流水账</button>
        <button type="button" class="active">小作文</button>
      </div>
      <article class="diary-entry with-tabs">
        <h2>今天我听见了春天</h2>
        <p>
          今天我们走到了有树影的路边。阳光先落在我的叶子上，然后慢慢变暖。
          我听见了车轮、脚步和风。你拍下的绿色像新的叶尖，提醒我真实世界一直在悄悄长大。
        </p>
        <div class="button-row" style="margin-top:24px;">
          <button type="button" class="button primary" data-go="share">分享</button>
          <button type="button" class="button secondary" data-go="diary">返回日历</button>
        </div>
      </article>
      ${nav("diary")}
    </section>
  `;
}

function mapScreen() {
  return `
    <section class="screen">
      ${header({ title: "散步地图", subtitle: "这个月和小芽走过的地方", back: true })}
      <article class="diary-entry">
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
        <h2 class="prompt-title small">国权路的咖啡店换了新菜单，我们走路过去看看路边的银杏吧！</h2>
        <div class="button-row">
          <button type="button" class="button primary" data-start-walk="local">去看看</button>
          <button type="button" class="button secondary" data-go="home">返回首页</button>
        </div>
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
        ${sprout("happy")}
        <div class="profile-text">
          <h2>Chiara的小芽</h2>
          <p>成长 18 天 · 今日很想见光</p>
          <p>已一起散步7次</p>
          <p>成长值：65</p>
          <div class="growth"><span></span></div>
        </div>
      </article>
      <article class="settings-card">
        ${["设备状态", "语音音色", "提醒时间", "数据权限"].map((label) => `<div class="setting-row"><span>${label}</span><span>›</span></div>`).join("")}
      </article>
      <article class="atlas-entry" data-go="atlas">
        <h2>我解锁的小芽 <span style="float:right;color:#9aa08e;">›</span></h2>
        <div class="badges"><span class="badge"></span><span class="badge"></span><span class="badge"></span><span class="badge"></span></div>
      </article>
      ${nav("me")}
    </section>
  `;
}

function atlasScreen() {
  const sprouts = [
    ["Light Sprout", "已解锁", false],
    ["Color Sprout", "已解锁", false],
    ["City Sprout", "已解锁", false],
    ["Sound Sprout", "已解锁", false],
    ["Night Sprout", "去散步解锁", true],
    ["Bloom Sprout", "去散步解锁", true],
  ];

  return `
    <section class="screen">
      ${header({ title: "小芽图鉴", subtitle: "收集不同的小芽形态", back: true })}
      <div class="atlas-grid">
        ${sprouts
          .map(
            ([name, status, locked]) => `
              <article class="atlas-card ${locked ? "locked" : ""}">
                ${tinySprout()}
                <h3>${name}</h3>
                <p>${status}</p>
              </article>
            `
          )
          .join("")}
      </div>
      ${nav("me")}
    </section>
  `;
}

function shareScreen() {
  return `
    <section class="screen">
      ${header({ title: "分享卡片", subtitle: "把今天的小芽日记发给朋友", back: true })}
      <article class="share-card">
        <div class="share-hero">
          ${sprout("happy")}
          <span class="share-sun"></span>
          <span class="share-tree"></span>
        </div>
        <h2>今天我听见了春天</h2>
        <p>32 min walk · 3 sounds · 5 greens</p>
        <p style="margin-top:18px;">刚才的光不是屏幕里的光。它落在我的叶子上，也落在你的鞋尖前面。</p>
        <span class="qr" aria-hidden="true"></span>
      </article>
      <div class="button-row" style="position:absolute;left:36px;top:632px;margin:0;gap:28px;">
        <button type="button" class="button primary wide" data-save-share>保存图片</button>
        <button type="button" class="button secondary wide" data-save-share>分享</button>
      </div>
      <article class="privacy-card">
        <h3>隐私提示</h3>
        <p>分享卡片默认不包含精确位置。</p>
      </article>
    </section>
  `;
}

function lightTaskScreen(inviteKey) {
  return `
    <section class="screen">
      ${header({ title: "换个更轻松的", subtitle: "降低任务强度", back: true })}
      ${sensors(false)}
      <article class="prompt-card">
        <p class="prompt-label">Level 0</p>
        <h2 class="prompt-title">那我们先去窗边吧。</h2>
        <div class="button-row">
          <button type="button" class="button primary" data-start-walk="${inviteKey}">开始</button>
          <button type="button" class="button secondary" data-go="${INVITES[inviteKey].screen}">返回</button>
        </div>
      </article>
      ${sprout("wilted")}
      ${nav("walk")}
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
    pause: pauseScreen,
    finish: finishScreen,
    diary: diaryScreen,
    "diary-log": diaryLogScreen,
    "diary-essay": diaryEssayScreen,
    map: mapScreen,
    nearby: nearbyScreen,
    me: meScreen,
    atlas: atlasScreen,
    share: shareScreen,
    lighter: () => lightTaskScreen(state.currentInvite),
  };

  const screen = routes[state.route] || homeScreen;
  app.innerHTML = screen();
}

document.addEventListener("click", (event) => {
  const random = event.target.closest("[data-random-invite]");
  const start = event.target.closest("[data-start-walk]");
  const lighter = event.target.closest("[data-lighter]");
  const go = event.target.closest("[data-go]");
  const back = event.target.closest("[data-back]");
  const devGo = event.target.closest("[data-dev-go]");
  const saveShare = event.target.closest("[data-save-share]");

  if (random) {
    randomInvite();
    return;
  }

  if (start) {
    startWalk(start.dataset.startWalk);
    return;
  }

  if (lighter) {
    state.currentInvite = lighter.dataset.lighter;
    setRoute("lighter");
    return;
  }

  if (back) {
    if (state.route.startsWith("walk-")) setRoute("pause");
    else setRoute(state.previousRoute || "home");
    return;
  }

  if (go) {
    setRoute(go.dataset.go);
    return;
  }

  if (devGo) {
    setRoute(devGo.dataset.devGo);
    return;
  }

  if (saveShare) {
    saveShare.textContent = "已保存";
  }
});

render();
