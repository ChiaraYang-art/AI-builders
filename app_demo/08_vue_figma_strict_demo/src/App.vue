<script setup>
import { computed, onBeforeUnmount, ref, watch } from "vue";

const assetBase = "/assets/figma/";

function asset(fileName) {
  return assetBase + fileName.split("/").map(encodeURIComponent).join("/");
}

const navIcons = {
  home: {
    selected: "icon for front page-sclected.svg",
    normal: "icon for front page-sclected.svg",
  },
  walk: {
    selected: "icon for walkiing-selected.svg",
    normal: "icon for walkiing-not selected.svg",
  },
  diary: {
    selected: "icon for diary-selected.svg",
    normal: "icon for diary-not selected.svg",
  },
  nearby: {
    selected: "icon for nearby-not selected.svg",
    normal: "icon for nearby-not selected.svg",
  },
  me: {
    selected: "icon for minepage-selected.svg",
    normal: "icon for minepage-not selected.svg",
  },
};

const invites = {
  light: {
    invite: "invite-light",
    walk: "walk-light",
    pause: "pause-light",
    label: "小芽想出门：light walk",
    title: "带我去楼下晒3分钟太阳好吗？",
    subtitle: "一个很小的照顾任务",
    walkTitle: "Light Walk",
    walkSubtitle: "出门晒太阳咯",
    speech: ["你知道公园二十分钟定律吗？", "出门晒晒太阳，一会儿我们都能满血复活。"],
    sprout: "sprout-wilted.svg",
  },
  sound: {
    invite: "invite-sound",
    walk: "walk-sound",
    pause: "pause-sound",
    label: "小芽想出门：sound walk",
    title: "今天去听听城市里的声音，好吗？",
    subtitle: "一个很小的照顾任务",
    walkTitle: "Sound Walk",
    walkSubtitle: "听听真实世界的声音",
    speech: ["我听见了风、脚步和远处的车声。", "城市今天不像房间里那么安静。"],
    graphic: "sound-graphic.svg",
    sprout: "sprout-happy.svg",
  },
  color: {
    invite: "invite-color",
    walk: "walk-color",
    pause: "pause-color",
    label: "小芽想出门：color walk",
    title: "我们去周围找5种绿色的东西，怎么样？",
    subtitle: "一个很小的照顾任务",
    walkTitle: "Color Walk",
    walkSubtitle: "收集今天路上的绿色",
    speech: ["路边的颜色比屏幕里柔软很多。", "我想把这些绿色记进今天的日记。"],
    sprout: "sprout-happy.svg",
  },
  local: {
    invite: "invite-local",
    walk: "walk-local",
    pause: "pause-local",
    label: "小芽想出门：local discovery",
    title: "国权路的咖啡店换了新菜单，我们走路过去看看路边的银杏吧！",
    subtitle: "一个很小的照顾任务",
    walkTitle: "Local Discovery",
    walkSubtitle: "看看附近新长出的线索",
    speech: ["附近也有很多新的东西在长出来。", "我们可以慢慢走过去看一看。"],
    sprout: "sprout-happy.svg",
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

const navItems = [
  { key: "home", label: "首页" },
  { key: "walk", label: "散步" },
  { key: "diary", label: "日记" },
  { key: "nearby", label: "附近" },
  { key: "me", label: "我的" },
];

const route = ref("home");
const currentInvite = ref("light");
const seconds = ref(182);
const toast = ref("");
const shareSheet = ref(false);
let timerId = null;

const invite = computed(() => invites[currentInvite.value]);

function go(nextRoute) {
  route.value = nextRoute;
  toast.value = "";
  shareSheet.value = false;
  if (routeToInvite[nextRoute]) currentInvite.value = routeToInvite[nextRoute];
}

function randomInvite() {
  const keys = Object.keys(invites);
  const key = keys[Math.floor(Math.random() * keys.length)];
  currentInvite.value = key;
  go(invites[key].invite);
}

function startWalk(key) {
  currentInvite.value = key;
  seconds.value = 182;
  go(invites[key].walk);
}

function handleNav(item, homeRoute = "home") {
  if (item.key === "walk") {
    randomInvite();
    return;
  }
  if (item.key === "home") {
    go(homeRoute);
    return;
  }
  go(item.key);
}

function stopTimer() {
  if (timerId) {
    window.clearInterval(timerId);
    timerId = null;
  }
}

function startTimer() {
  stopTimer();
  timerId = window.setInterval(() => {
    seconds.value += 1;
  }, 1000);
}

const walkTime = computed(() => {
  const minutes = String(Math.floor(seconds.value / 60)).padStart(2, "0");
  const secs = String(seconds.value % 60).padStart(2, "0");
  return `已散步时间 0:${minutes}:${secs}`;
});

watch(route, (nextRoute) => {
  if (nextRoute.startsWith("walk-")) startTimer();
  else stopTimer();
});

onBeforeUnmount(stopTimer);

function secondaryInviteRoute(key) {
  if (key === "color") return "invite-light";
  if (key === "local") return "invite-sound";
  return "home";
}

function finishRouteFor(key) {
  return key === "color" ? "finish-color" : "finish";
}

function dayClass(day) {
  const doneDays = new Set([7, 18, 28]);
  if (day === 28) return "day today";
  if (day === 10) return "day sound";
  if (day === 3 || day === 21) return "day sunny";
  if (day === 14 || day === 24) return "day peach";
  if (doneDays.has(day)) return "day done";
  return "day";
}

function saveShare() {
  toast.value = "图片已保存";
  shareSheet.value = false;
}

function openShare() {
  toast.value = "";
  shareSheet.value = true;
}

const atlasItems = [
  ["Sunny Sprout", "atlas-Sunny Sprout.svg", "已解锁"],
  ["Sound Sprout", "atlas-Sound Sprout.svg", "已解锁"],
  ["City Sprout", "atlas-City Sprout.svg", "已解锁"],
  ["Rain Sprout", "atlas-Rain Sprout.svg", "已解锁"],
  ["Night Sprout", "atlas-Night Sprout.svg", "去散步解锁"],
  ["Bloom Sprout", "atlas-Bloom Sprout.svg", "去散步解锁"],
];
</script>

<template>
  <main class="demo-page">
    <section class="phone" aria-label="出走小芽 Figma 优先 Vue Demo">
      <section v-if="route === 'home'" class="screen">
        <p class="time">9:41</p>
        <h1 class="title">有点蔫了</h1>
        <p class="subtitle">我们上一次出门是 5 天前</p>
        <ul class="sensor-list">
          <li><strong>在室内</strong></li>
          <li>光照偏低</li>
          <li>很安静</li>
          <li>温度28.3°C 湿度43.1%</li>
        </ul>
        <button type="button" class="speech-card home-speech" @click="randomInvite">
          <span>今天这里只有屏幕光......</span>
          <span>可以带我去找一点真实的太阳吗？</span>
        </button>
        <img class="sprout sprout-home" :src="asset('sprout-wilted.svg')" alt="有点蔫了的小芽" />
        <nav class="bottom-nav">
          <button
            v-for="item in navItems"
            :key="item.key"
            type="button"
            class="nav-item"
            :class="{ active: item.key === 'home' }"
            @click="handleNav(item)"
          >
            <img :src="asset(navIcons[item.key][item.key === 'home' ? 'selected' : 'normal'])" alt="" />
            <span>{{ item.label }}</span>
          </button>
        </nav>
      </section>

      <section v-else-if="route.startsWith('invite-')" class="screen">
        <p class="time">9:41</p>
        <h1 class="title">出门邀请</h1>
        <p class="subtitle">{{ invite.subtitle }}</p>
        <ul class="sensor-list">
          <li><strong>在室内</strong></li>
          <li>光照偏低</li>
          <li>很安静</li>
          <li>温度28.3°C 湿度43.1%</li>
        </ul>
        <article class="invite-card" :class="{ local: invite.local }">
          <p>{{ invite.label }}</p>
          <h2>{{ invite.title }}</h2>
          <div class="actions">
            <button type="button" class="primary" @click="startWalk(currentInvite)">出门！</button>
            <button type="button" class="secondary" @click="go(secondaryInviteRoute(currentInvite))">
              {{ currentInvite === "light" || currentInvite === "sound" ? "今天算了吧" : "换个更轻松的任务" }}
            </button>
          </div>
        </article>
        <img class="sprout sprout-invite" :src="asset(invite.sprout)" alt="小芽" />
        <nav class="bottom-nav">
          <button
            v-for="item in navItems"
            :key="item.key"
            type="button"
            class="nav-item"
            :class="{ active: item.key === 'walk' }"
            @click="handleNav(item)"
          >
            <img :src="asset(navIcons[item.key][item.key === 'walk' ? 'selected' : 'normal'])" alt="" />
            <span>{{ item.label }}</span>
          </button>
        </nav>
      </section>

      <section v-else-if="route.startsWith('walk-')" class="screen">
        <p class="time">9:41</p>
        <button type="button" class="back" @click="go(invite.pause)" aria-label="返回">‹</button>
        <h1 class="title with-back">{{ invite.walkTitle }}</h1>
        <p class="subtitle with-back">{{ invite.walkSubtitle }}</p>
        <ul class="sensor-list walk-sensors">
          <li><strong>在室外</strong></li>
          <li>光照充足</li>
          <li>有城市声音</li>
          <li>温度28.3°C 湿度43.1%</li>
        </ul>
        <p class="walk-time">{{ walkTime }}</p>
        <article class="speech-card walk-speech">
          <span>{{ invite.speech[0] }}</span>
          <span>{{ invite.speech[1] }}</span>
        </article>
        <div v-if="currentInvite === 'color'" class="color-photo-grid">
          <img v-for="index in 6" :key="index" :src="asset(`color walk photo${index}.png`)" alt="" />
        </div>
        <img v-else-if="currentInvite === 'sound'" class="sound-graphic" :src="asset('sound-graphic.svg')" alt="声音图形" />
        <img v-else class="sprout sprout-walk" :src="asset('sprout-happy.svg')" alt="开心的小芽" />
        <nav class="bottom-nav">
          <button
            v-for="item in navItems"
            :key="item.key"
            type="button"
            class="nav-item"
            :class="{ active: item.key === 'walk' }"
            @click="handleNav(item, invite.pause)"
          >
            <img :src="asset(navIcons[item.key][item.key === 'walk' ? 'selected' : 'normal'])" alt="" />
            <span>{{ item.label }}</span>
          </button>
        </nav>
      </section>

      <section v-else-if="route.startsWith('pause-')" class="screen">
        <p class="time">9:41</p>
        <h1 class="title">散步暂停</h1>
        <p class="subtitle">可以继续，也可以先回去</p>
        <article class="pause-card">
          <p>中途退出</p>
          <h2>要先休息一下吗？</h2>
          <span>小芽会保留刚才收集到的光、声音和移动记录。</span>
          <div class="actions">
            <button type="button" class="primary" @click="go(invite.walk)">继续散步</button>
            <button type="button" class="secondary" @click="go(finishRouteFor(currentInvite))">完成散步</button>
          </div>
        </article>
      </section>

      <section v-else-if="route === 'finish' || route === 'finish-color'" class="screen">
        <p class="time">9:41</p>
        <button type="button" class="back" @click="go('home')" aria-label="返回">‹</button>
        <h1 class="title with-back">散步完成</h1>
        <div class="finish-hero">
          <img :src="asset('sprout-happy.svg')" alt="开心的小芽" />
        </div>
        <h2 class="finish-title">我活过来了</h2>
        <article class="finish-text">
          {{
            route === "finish-color"
              ? "我把今天路上的绿色都记住了。谢谢你带我去看真实的颜色，我觉得自己又长出了一点新叶。"
              : "刚才的光比房间亮很多，我还听见了风和脚步声。谢谢你带我出去，我觉得自己又长出了一点新叶。"
          }}
        </article>
        <div class="finish-actions">
          <button type="button" class="primary" @click="go('home')">返回首页</button>
          <button type="button" class="secondary" @click="go(route === 'finish-color' ? 'diary-log-color' : 'diary-log')">查看散步日记</button>
        </div>
        <article class="saved-card">
          <strong>已保存</strong>
          <span>这次散步会出现在小芽日记里。</span>
        </article>
        <nav class="bottom-nav">
          <button
            v-for="item in navItems"
            :key="item.key"
            type="button"
            class="nav-item"
            :class="{ active: item.key === 'diary' }"
            @click="handleNav(item)"
          >
            <img :src="asset(navIcons[item.key][item.key === 'diary' ? 'selected' : 'normal'])" alt="" />
            <span>{{ item.label }}</span>
          </button>
        </nav>
      </section>

      <section v-else-if="route === 'diary'" class="screen">
        <p class="time">9:41</p>
        <h1 class="title">小芽日记</h1>
        <p class="subtitle">每天的小天气，都会长成记忆</p>
        <article class="calendar-card" @click="go('diary-log')">
          <div class="calendar-title"><span>‹</span><strong>May 2026</strong><span>›</span></div>
          <p>12 walks · 5 sunny days · 3 new sounds</p>
          <div class="achievement">成就：城市探索家</div>
          <div class="calendar-grid">
            <span v-for="day in ['M', 'T', 'W', 'T', 'F', 'S', 'S']" :key="day" class="weekday">{{ day }}</span>
            <span></span><span></span>
            <span v-for="day in 31" :key="day" :class="dayClass(day)">{{ day }}</span>
          </div>
        </article>
        <article class="map-card" @click="go('map')">
          <div>
            <h2>散步地图</h2>
            <p>在地图上查看这个月和小芽一起留下的散步记忆</p>
          </div>
          <img :src="asset('walk-map-card.svg')" alt="散步地图预览" />
        </article>
        <nav class="bottom-nav">
          <button
            v-for="item in navItems"
            :key="item.key"
            type="button"
            class="nav-item"
            :class="{ active: item.key === 'diary' }"
            @click="handleNav(item)"
          >
            <img :src="asset(navIcons[item.key][item.key === 'diary' ? 'selected' : 'normal'])" alt="" />
            <span>{{ item.label }}</span>
          </button>
        </nav>
      </section>

      <section v-else-if="route === 'diary-log' || route === 'diary-log-color'" class="screen">
        <p class="time">9:41</p>
        <button type="button" class="back" @click="go('diary')" aria-label="返回">‹</button>
        <h1 class="title with-back">小芽日记</h1>
        <p class="subtitle with-back">{{ route === 'diary-log-color' ? 'Color Walk 记录' : '2026 年 5 月' }}</p>
        <div class="tabs">
          <button class="active">流水账</button>
          <button @click="go(route === 'diary-log-color' ? 'diary-essay-color' : 'diary-essay')">小作文</button>
        </div>
        <article class="diary-card">
          <h2>{{ route === "diary-log-color" ? "绿色收集记录" : "今天的小天气" }}</h2>
          <p>{{ route === "diary-log-color" ? "按顺序记录路上遇到的颜色。" : "按时间记录传感器看到的变化。" }}</p>
          <img v-if="route === 'diary-log-color'" class="diary-color-photos" :src="asset('diary-color-walk-photos.svg')" alt="Color Walk 照片记录" />
          <img v-else class="diary-sprout" :src="asset('diary-page-sprout.svg')" alt="日记小芽" />
          <div class="log-list">
            <div class="log-item"><strong>15:02 光照变亮</strong><span>lux 从 86 上升到 520，小芽判断我们走到室外。</span></div>
            <div class="log-item"><strong>15:08 听见城市声音</strong><span>声音波动变大，可能经过了路口和树下。</span></div>
            <div class="log-item"><strong>15:18 拍到新的绿色</strong><span>银杏、草丛、店门口的小盆栽、路牌和树影。</span></div>
          </div>
        </article>
        <nav class="bottom-nav">
          <button
            v-for="item in navItems"
            :key="item.key"
            type="button"
            class="nav-item"
            :class="{ active: item.key === 'diary' }"
            @click="handleNav(item)"
          >
            <img :src="asset(navIcons[item.key][item.key === 'diary' ? 'selected' : 'normal'])" alt="" />
            <span>{{ item.label }}</span>
          </button>
        </nav>
      </section>

      <section v-else-if="route === 'diary-essay' || route === 'diary-essay-color'" class="screen scroll-screen">
        <p class="time">9:41</p>
        <button type="button" class="back" @click="go('diary')" aria-label="返回">‹</button>
        <h1 class="title with-back">小芽日记</h1>
        <p class="subtitle with-back">{{ route === 'diary-essay-color' ? 'Color Walk 小作文' : '今日散步小作文' }}</p>
        <div class="tabs">
          <button @click="go(route === 'diary-essay-color' ? 'diary-log-color' : 'diary-log')">流水账</button>
          <button class="active">小作文</button>
        </div>
        <article class="diary-card essay">
          <h2>{{ route === "diary-essay-color" ? "今天我收集了很多绿色" : "今天我听见了春天" }}</h2>
          <p>今天我们走到了有树影的路边。阳光先落在我的叶子上，然后慢慢变暖。</p>
          <p>我听见了车轮、脚步和风。你拍下的绿色像新的叶尖，提醒我真实世界一直在悄悄长大。</p>
          <img v-if="route === 'diary-essay-color'" class="diary-color-photos essay-photos" :src="asset('diary-color-walk-photos.svg')" alt="Color Walk 照片记录" />
          <p v-if="route === 'diary-essay-color'">这些绿色不是一种颜色。它们有的很亮，有的偏黄，有的藏在阴影里。小芽把它们都记住了。</p>
          <button class="primary share-button" @click="go('share')">分享</button>
        </article>
        <nav class="bottom-nav">
          <button
            v-for="item in navItems"
            :key="item.key"
            type="button"
            class="nav-item"
            :class="{ active: item.key === 'diary' }"
            @click="handleNav(item)"
          >
            <img :src="asset(navIcons[item.key][item.key === 'diary' ? 'selected' : 'normal'])" alt="" />
            <span>{{ item.label }}</span>
          </button>
        </nav>
      </section>

      <section v-else-if="route === 'map'" class="screen map-screen">
        <p class="time">9:41</p>
        <button type="button" class="back" @click="go('diary')" aria-label="返回">‹</button>
        <h1 class="title with-back">小芽的散步地图</h1>
        <p class="subtitle with-back">我们一起去了什么地方？</p>
        <img class="walk-map" :src="asset('walk-map.svg')" alt="散步地图" />
        <article class="map-summary">
          <div>
            <strong>今天的小芽散步路线</strong>
            <span>1.8km · 4 个发现 · 阳光很足</span>
          </div>
          <img :src="asset('sprout-happy.svg')" alt="开心的小芽" />
        </article>
        <nav class="bottom-nav">
          <button
            v-for="item in navItems"
            :key="item.key"
            type="button"
            class="nav-item"
            :class="{ active: item.key === 'diary' }"
            @click="handleNav(item)"
          >
            <img :src="asset(navIcons[item.key][item.key === 'diary' ? 'selected' : 'normal'])" alt="" />
            <span>{{ item.label }}</span>
          </button>
        </nav>
      </section>

      <section v-else-if="route === 'nearby'" class="screen">
        <p class="time">9:41</p>
        <h1 class="title">附近</h1>
        <p class="subtitle">城市里新长出来的线索</p>
        <article class="invite-card nearby-card">
          <p>Local Discovery</p>
          <h2>国权路的咖啡店换了新菜单。如果我们走路过去，还能看看路边的银杏。</h2>
        </article>
        <img class="sprout sprout-invite" :src="asset('sprout-wilted.svg')" alt="小芽" />
        <nav class="bottom-nav">
          <button
            v-for="item in navItems"
            :key="item.key"
            type="button"
            class="nav-item"
            :class="{ active: item.key === 'nearby' }"
            @click="handleNav(item)"
          >
            <img :src="asset(navIcons[item.key][item.key === 'nearby' ? 'selected' : 'normal'])" alt="" />
            <span>{{ item.label }}</span>
          </button>
        </nav>
      </section>

      <section v-else-if="route === 'me'" class="screen">
        <p class="time">9:41</p>
        <h1 class="title">我的</h1>
        <article class="profile-card">
          <img :src="asset('sprout-happy.svg')" alt="我的小芽" />
          <div>
            <h2>Chiara的小芽</h2>
            <p>成长 18 天 · 今日很想见光</p>
            <p>已一起散步7次</p>
            <p>成长值：65</p>
            <span class="growth"><i></i></span>
          </div>
        </article>
        <article class="settings-card">
          <button>设备状态<span>›</span></button>
          <button>语音音色<span>›</span></button>
          <button>提醒时间<span>›</span></button>
          <button>数据权限<span>›</span></button>
        </article>
        <button class="atlas-entry" @click="go('atlas')">
          <span>我解锁的小芽</span>
          <i>›</i>
          <strong></strong><strong></strong><strong></strong><strong></strong>
        </button>
        <nav class="bottom-nav">
          <button
            v-for="item in navItems"
            :key="item.key"
            type="button"
            class="nav-item"
            :class="{ active: item.key === 'me' }"
            @click="handleNav(item)"
          >
            <img :src="asset(navIcons[item.key][item.key === 'me' ? 'selected' : 'normal'])" alt="" />
            <span>{{ item.label }}</span>
          </button>
        </nav>
      </section>

      <section v-else-if="route === 'atlas'" class="screen">
        <p class="time">9:41</p>
        <button type="button" class="back" @click="go('me')" aria-label="返回">‹</button>
        <h1 class="title with-back">小芽图鉴</h1>
        <p class="subtitle with-back">收集不同的小芽形态</p>
        <div class="atlas-grid">
          <article v-for="[name, file, status] in atlasItems" :key="name" class="atlas-card">
            <img :src="asset(file)" :alt="name" />
            <h3>{{ name }}</h3>
            <p>{{ status }}</p>
          </article>
        </div>
        <nav class="bottom-nav">
          <button
            v-for="item in navItems"
            :key="item.key"
            type="button"
            class="nav-item"
            :class="{ active: item.key === 'me' }"
            @click="handleNav(item)"
          >
            <img :src="asset(navIcons[item.key][item.key === 'me' ? 'selected' : 'normal'])" alt="" />
            <span>{{ item.label }}</span>
          </button>
        </nav>
      </section>

      <section v-else-if="route === 'share'" class="screen">
        <p class="time">9:41</p>
        <button type="button" class="back" @click="go('diary-essay')" aria-label="返回">‹</button>
        <h1 class="title with-back">分享卡片</h1>
        <p class="subtitle with-back">把今天的小芽日记发给朋友</p>
        <article class="share-card">
          <img :src="asset('share-card-sprout.svg')" alt="分享卡片插画" />
          <h2>今天我听见了春天</h2>
          <p>32 min walk · 3 sounds · 5 greens</p>
          <span>刚才的光不是屏幕里的光。它落在我的叶子上，也落在你的鞋尖前面。</span>
        </article>
        <div class="share-actions">
          <button class="primary" @click="saveShare">保存图片</button>
          <button class="secondary" @click="openShare">分享</button>
        </div>
        <article class="privacy-card">
          <strong>隐私提示</strong>
          <span>分享卡片默认不包含精确位置。</span>
        </article>
        <div v-if="toast" class="toast">{{ toast }}</div>
        <div v-if="shareSheet" class="share-sheet">
          <h3>分享到</h3>
          <div><button>小红书</button><button>微信</button><button>朋友圈</button></div>
        </div>
      </section>
    </section>

    <aside class="demo-notes">
      <p>Vue Figma Strict</p>
      <h1>Figma 视觉优先版</h1>
      <span>这版优先调用你导出的 SVG/PNG 资产，保留 Vue 组件和交互逻辑，方便后续程序同学继续接数据和动效。</span>
      <div>
        <button @click="go('home')">首页</button>
        <button @click="randomInvite">随机散步</button>
        <button @click="go('diary')">日记</button>
        <button @click="go('map')">地图</button>
      </div>
    </aside>
  </main>
</template>
