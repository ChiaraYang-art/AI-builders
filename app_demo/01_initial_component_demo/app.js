const INVITES = {
  light: {
    title: "带我去楼下晒3分钟太阳好嘛？",
    cancel: "今天算了吧",
    local: false,
  },
  sound: {
    title: "今天天气一般 或许我们出门听听城市的声音？",
    cancel: "今天算了吧",
    local: false,
  },
  color: {
    title: "我们去周围找 5 种绿色的东西，怎么样？",
    cancel: "换个更轻松的",
    local: false,
  },
  local: {
    title: "国权路的咖啡店换了新菜单，如果我们走路过去，还能看看路边的银杏！",
    cancel: "换个更轻松的",
    local: true,
  },
};

const state = {
  screen: "home",
  previous: "home",
  invite: "light",
  seconds: 0,
  timer: null,
};

const app = document.querySelector("#app");

function randomInvite() {
  const keys = Object.keys(INVITES);
  state.invite = keys[Math.floor(Math.random() * keys.length)];
  go("invite");
}

function go(screen) {
  state.previous = state.screen;
  state.screen = screen;
  stopTimer();
  render();
}

function back() {
  go(state.previous || "home");
}

function startTimer() {
  stopTimer();
  state.seconds = 0;
  state.timer = window.setInterval(() => {
    state.seconds += 1;
    const el = document.querySelector("[data-timer]");
    if (el) el.textContent = formatTime(state.seconds);
  }, 1000);
}

function stopTimer() {
  if (state.timer) {
    window.clearInterval(state.timer);
    state.timer = null;
  }
}

function formatTime(seconds) {
  const minute = String(Math.floor(seconds / 60)).padStart(2, "0");
  const second = String(seconds % 60).padStart(2, "0");
  return `${minute}:${second}`;
}

function sproutStage() {
  return `
    <div class="sprout-stage" aria-label="小芽插画">
      <div class="sprout-base">
        <span class="soil-layer"></span>
        <span class="soil-layer"></span>
        <span class="soil-layer"></span>
        <span class="soil-layer"></span>
        <span class="soil-layer"></span>
        <span class="eye-slit left"></span>
        <span class="eye-slit right"></span>
      </div>
      ${flower("one")}
      ${flower("two")}
      ${flower("three")}
    </div>
  `;
}

function flower(name) {
  return `
    <div class="flower ${name}">
      <span class="petal p1"></span>
      <span class="petal p2"></span>
      <span class="petal p3"></span>
      <span class="petal p4"></span>
      <span class="petal p5"></span>
      <span class="flower-center"></span>
    </div>
  `;
}

function sensors() {
  return `
    <div class="sensor-copy">
      <span>在室内</span>
      <span>光照偏低</span>
      <span>很安静</span>
      <span>温度28.3℃ 湿度43.1%</span>
    </div>
  `;
}

function header(title, subtitle) {
  return `
    <p class="t-time">9:41</p>
    <h1 class="title">${title}</h1>
    <p class="subtitle">${subtitle}</p>
  `;
}

function nav(active = "home") {
  const items = [
    ["home", "⌘", "首页"],
    ["walk", "♙", "散步"],
    ["diary", "▣", "日记"],
    ["nearby", "⌖", "附近"],
    ["me", "○", "我的"],
  ];

  return `
    <nav class="bottom-nav" aria-label="底部导航">
      ${items
        .map(
          ([key, icon, label]) => `
            <button class="nav-item ${key === active ? "active" : ""}" data-jump="${key}">
              <span class="nav-icon">${icon}</span>
              ${label}
            </button>
          `
        )
        .join("")}
    </nav>
  `;
}

function baseScreen(content, active = "home") {
  return `<div class="screen">${content}${nav(active)}</div>`;
}

const screens = {
  home() {
    return baseScreen(`
      ${header("有点蔫了", "我们上一次出门是 5 天前")}
      ${sensors()}
      <button class="speech-card" data-action="randomInvite" aria-label="点击进入随机出门邀请">
        <p>今天这里只有屏幕光......</p>
        <p>可以带我去找一点真实的太阳吗？</p>
      </button>
      ${sproutStage()}
    `);
  },

  invite() {
    const invite = INVITES[state.invite];
    return baseScreen(`
      ${header("出门邀请", "一个很小的照顾任务")}
      ${sensors()}
      <section class="prompt-card ${invite.local ? "local" : ""}">
        <p class="prompt-kicker">小芽想出门</p>
        <h2 class="prompt-title ${invite.local ? "local" : ""}">${invite.title}</h2>
        <div class="prompt-actions">
          <button class="figma-button primary" data-jump="walk">出门！</button>
          <button class="figma-button secondary" data-jump="lighter">${invite.cancel}</button>
        </div>
      </section>
      ${sproutStage()}
    `);
  },

  walk() {
    window.setTimeout(startTimer, 0);
    return baseScreen(`
      ${header("散步进行中", "小芽正在感受真实世界")}
      ${sensors()}
      <section class="walk-panel">
        <p class="prompt-kicker">正在收集</p>
        <h2 data-timer>${formatTime(state.seconds)}</h2>
        <p>如果光照、声音、移动里有两项变丰富，小芽就会判断我们真的出门了。</p>
        <div class="metric-row">
          <span class="metric">光照<br>+18</span>
          <span class="metric">声音<br>+12</span>
          <span class="metric">移动<br>+24</span>
        </div>
        <div class="prompt-actions">
          <button class="figma-button secondary" data-jump="pause">暂停</button>
          <button class="figma-button primary" data-jump="finish">结束</button>
        </div>
      </section>
      ${sproutStage()}
    `, "walk");
  },

  pause() {
    return baseScreen(`
      ${header("散步暂停", "可以继续，也可以先回去")}
      ${sensors()}
      <section class="notice">
        <p class="prompt-kicker">中途退出</p>
        <h2>要先休息一下吗？</h2>
        <p>小芽会保留刚才收集到的光、声音和移动记录。今天不完成也没关系。</p>
        <div class="prompt-actions">
          <button class="figma-button primary" data-jump="walk">继续</button>
          <button class="figma-button secondary" data-jump="home">退出</button>
        </div>
      </section>
      ${sproutStage()}
    `, "walk");
  },

  finish() {
    return baseScreen(`
      ${header("散步完成", "小芽把刚才的感受说出来")}
      ${sensors()}
      <section class="prompt-card">
        <p class="prompt-kicker">小芽说</p>
        <h2 class="prompt-title">刚才的光比房间亮很多。</h2>
        <p style="margin:10px 0 0;color:#79886e;font-size:14px;line-height:22px;font-weight:650;">
          我还听见了风和脚步声。谢谢你带我出去，我觉得自己又长出了一点新叶。
        </p>
        <div class="prompt-actions">
          <button class="figma-button primary" data-jump="diary">写入日记</button>
          <button class="figma-button secondary" data-jump="share">分享</button>
        </div>
      </section>
      ${sproutStage()}
    `, "diary");
  },

  lighter() {
    return baseScreen(`
      ${header("换个更轻松的", "返回上一级或降低任务强度")}
      ${sensors()}
      <section class="notice">
        <p class="prompt-kicker">Level 0</p>
        <h2>那我们先去窗边吧。</h2>
        <p>不下楼也可以。带我到窗边听 30 秒外面的声音，就算今天照顾过小芽了。</p>
        <div class="prompt-actions">
          <button class="figma-button primary" data-jump="walk">开始</button>
          <button class="figma-button secondary" data-jump="invite">返回</button>
        </div>
      </section>
      ${sproutStage()}
    `, "walk");
  },

  diary() {
    return baseScreen(`
      ${header("小芽日记", "今天的散步已经被保存")}
      <section class="diary-card">
        <p class="prompt-kicker">2026年5月29日</p>
        <h2>我在路上听见了春天</h2>
        <p>
          今天我们走到了有树影的路边。阳光先落在我的叶子上，然后慢慢变暖。
          我听见了车轮、脚步和风。你拍下的绿色像新的叶尖。
        </p>
        <div class="prompt-actions">
          <button class="figma-button primary" data-jump="share">分享</button>
          <button class="figma-button secondary" data-jump="home">回首页</button>
        </div>
      </section>
    `, "diary");
  },

  share() {
    return baseScreen(`
      ${header("分享卡片", "把今天的小芽日记发给朋友")}
      <section class="share-card">
        <div class="share-visual">${sproutStage()}</div>
        <h2>今天我听见了春天</h2>
        <p>32 min walk · 3 sounds · 5 greens</p>
        <p style="margin-top:18px;">刚才的光不是屏幕里的光。它落在我的叶子上，也落在你的鞋尖前面。</p>
        <div class="prompt-actions">
          <button class="figma-button primary" data-action="saved">保存图片</button>
          <button class="figma-button secondary" data-jump="diary">返回</button>
        </div>
      </section>
    `, "diary");
  },

  nearby() {
    return baseScreen(`
      ${header("附近", "城市里新长出来的线索")}
      <section class="notice">
        <p class="prompt-kicker">Local Discovery</p>
        <h2>国权路的咖啡店换了新菜单。</h2>
        <p>如果我们走路过去，还能看看路边的银杏。这个任务不需要消费，只需要看一眼。</p>
        <div class="prompt-actions">
          <button class="figma-button primary" data-jump="walk">去看看</button>
          <button class="figma-button secondary" data-jump="home">回首页</button>
        </div>
      </section>
      ${sproutStage()}
    `, "nearby");
  },

  me() {
    return baseScreen(`
      ${header("我的", "设备、成长和设置")}
      <section class="notice">
        <p class="prompt-kicker">Alex 的小芽</p>
        <h2>成长 18 天</h2>
        <p>已解锁阳光小芽、声音小芽和街角小芽。晚上会自动进入安静模式。</p>
        <div class="prompt-actions">
          <button class="figma-button primary" data-jump="home">回首页</button>
        </div>
      </section>
      ${sproutStage()}
    `, "me");
  },
};

function render() {
  const view = screens[state.screen] || screens.home;
  app.innerHTML = view();
}

document.addEventListener("click", (event) => {
  const action = event.target.closest("[data-action]");
  const jump = event.target.closest("[data-jump]");

  if (action) {
    const name = action.dataset.action;
    if (name === "randomInvite") randomInvite();
    if (name === "saved") action.textContent = "已保存";
    return;
  }

  if (jump) {
    const target = jump.dataset.jump;
    if (target === "walk" && state.screen !== "walk") state.seconds = 0;
    go(target);
  }
});

document.querySelectorAll("[data-jump]").forEach((button) => {
  button.addEventListener("click", () => go(button.dataset.jump));
});

render();
