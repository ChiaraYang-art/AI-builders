<script setup>
import { computed, onBeforeUnmount, ref, watch } from "vue";
import BottomNav from "./components/BottomNav.vue";
import PhoneHeader from "./components/PhoneHeader.vue";
import SensorList from "./components/SensorList.vue";
import Sprout from "./components/Sprout.vue";

const invites = {
  light: {
    invite: "invite-light",
    walk: "walk-light",
    pause: "pause-light",
    label: "小芽想出门：Light Walk",
    title: "带我去楼下晒 3 分钟太阳，好吗？",
    walkTitle: "Light Walk",
    walkSubtitle: "出门晒太阳咯",
    speech: ["你知道公园二十分钟定律吗？", "出门晒晒太阳，一会儿我们都能满血复活。"],
  },
  sound: {
    invite: "invite-sound",
    walk: "walk-sound",
    pause: "pause-sound",
    label: "小芽想出门：Sound Walk",
    title: "今天去听听城市里的声音，好吗？",
    walkTitle: "Sound Walk",
    walkSubtitle: "听听真实世界的声音",
    speech: ["我听见了风、脚步和远处的车声。", "城市今天不像房间里那么安静。"],
  },
  color: {
    invite: "invite-color",
    walk: "walk-color",
    pause: "pause-color",
    label: "小芽想出门：Color Walk",
    title: "我们去周围找 5 种绿色的东西，怎么样？",
    walkTitle: "Color Walk",
    walkSubtitle: "收集今天路上的绿色",
    speech: ["路边的颜色比屏幕里柔软很多。", "我想把这些绿色记进今天的日记。"],
  },
  local: {
    invite: "invite-local",
    walk: "walk-local",
    pause: "pause-local",
    label: "小芽想出门：Local Discovery",
    title: "国权路的咖啡店换了新菜单，我们走路过去看看路边的银杏吧！",
    walkTitle: "Local Discovery",
    walkSubtitle: "看看附近新长出的线索",
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

function saveShare() {
  toast.value = "图片已保存";
  shareSheet.value = false;
}

function openShare() {
  toast.value = "";
  shareSheet.value = true;
}

const doneDays = new Set([3, 7, 10, 14, 18, 21, 24]);

function dayClass(day) {
  if (day === 28) return "day today";
  if (day === 10) return "day sound";
  if (day === 14 || day === 21) return "day sunny";
  if (doneDays.has(day)) return "day done";
  return "day";
}
</script>

<template>
  <main class="demo-page">
    <section class="phone" aria-label="出走小芽 Vue Demo">
      <section v-if="route === 'home'" class="screen">
        <PhoneHeader title="有点蔫了" subtitle="我们上一次出门是 5 天前" />
        <SensorList />
        <button type="button" class="speech-card" @click="randomInvite">
          <p>今天这里只剩屏幕光了......</p>
          <p>可以带我去找一点真实的太阳吗？</p>
        </button>
        <Sprout />
        <BottomNav active="home" @go="go" @random-invite="randomInvite" />
      </section>

      <section v-else-if="route.startsWith('invite-')" class="screen">
        <PhoneHeader title="出门邀请" subtitle="一个很小的照顾任务" />
        <SensorList />
        <article class="prompt-card" :class="{ local: invite.local }">
          <p class="prompt-label">{{ invite.label }}</p>
          <h2 class="prompt-title" :class="{ small: invite.local }">{{ invite.title }}</h2>
          <div class="button-row">
            <button type="button" class="button primary" @click="startWalk(currentInvite)">出门！</button>
            <button type="button" class="button secondary" @click="go(secondaryInviteRoute(currentInvite))">
              {{ currentInvite === "light" || currentInvite === "sound" ? "今天算了吧" : "换个更轻松的任务" }}
            </button>
          </div>
        </article>
        <Sprout />
        <BottomNav active="walk" @go="go" @random-invite="randomInvite" />
      </section>

      <section v-else-if="route.startsWith('walk-')" class="screen">
        <PhoneHeader :title="invite.walkTitle" :subtitle="invite.walkSubtitle" :back-route="invite.pause" @go="go" />
        <SensorList outdoor />
        <p class="walk-time">{{ walkTime }}</p>
        <article class="speech-card">
          <p>{{ invite.speech[0] }}</p>
          <p>{{ invite.speech[1] }}</p>
        </article>
        <Sprout mood="happy" variant="walk" />
        <BottomNav active="walk" :home-route="invite.pause" @go="go" @random-invite="randomInvite" />
      </section>

      <section v-else-if="route.startsWith('pause-')" class="screen">
        <PhoneHeader title="散步暂停" subtitle="可以继续，也可以先回去" />
        <article class="pause-card">
          <p class="prompt-label">中途退出</p>
          <h2>要先休息一下吗？</h2>
          <p>小芽会保留刚才收集到的光、声音和移动记录。</p>
          <div class="button-row">
            <button type="button" class="button primary" @click="go(invite.walk)">继续散步</button>
            <button type="button" class="button secondary" @click="go(finishRouteFor(currentInvite))">完成散步</button>
          </div>
        </article>
      </section>

      <section v-else-if="route === 'finish' || route === 'finish-color'" class="screen">
        <PhoneHeader title="散步完成" back-route="home" @go="go" />
        <div class="complete-hero"><Sprout mood="happy" /></div>
        <h2 class="complete-title">我活过来了</h2>
        <article class="complete-speech">
          {{
            route === "finish-color"
              ? "我把今天路上的绿色都记住了。谢谢你带我去看真实的颜色，我觉得自己又长出了一点新叶。"
              : "刚才的光比房间亮很多，我还听见了风和脚步声。谢谢你带我出去，我觉得自己又长出了一点新叶。"
          }}
        </article>
        <div class="button-row finish-actions">
          <button type="button" class="button primary wide" @click="go('home')">返回首页</button>
          <button type="button" class="button secondary wide" @click="go(route === 'finish-color' ? 'diary-log-color' : 'diary-log')">
            查看散步日记
          </button>
        </div>
        <article class="saved-card">
          <h3>已保存</h3>
          <p>这次散步会出现在小芽日记里。</p>
        </article>
        <BottomNav active="diary" @go="go" @random-invite="randomInvite" />
      </section>

      <section v-else-if="route === 'diary'" class="screen">
        <PhoneHeader title="小芽日记" subtitle="每天的小天气，都会长成记忆" />
        <article class="content-card calendar-card" @click="go('diary-log')">
          <div class="calendar-head"><span>‹</span><strong>May 2026</strong><span>›</span></div>
          <p class="calendar-summary">12 walks · 5 sunny days · 3 new sounds</p>
          <div class="achievement-chip">成就：城市探索家</div>
          <div class="calendar-grid">
            <span v-for="day in ['M', 'T', 'W', 'T', 'F', 'S', 'S']" :key="day" class="weekday">{{ day }}</span>
            <span></span><span></span><span></span>
            <span v-for="day in 31" :key="day" :class="dayClass(day)">{{ day }}</span>
          </div>
        </article>
        <article class="content-card map-card" @click="go('map')">
          <h2>散步地图</h2>
          <p>在地图上查看这个月和小芽一起留下的散步记忆</p>
          <div class="mini-map"></div>
        </article>
        <BottomNav active="diary" @go="go" @random-invite="randomInvite" />
      </section>

      <section v-else-if="route === 'diary-log' || route === 'diary-log-color'" class="screen">
        <PhoneHeader title="小芽日记" :subtitle="route === 'diary-log-color' ? 'Color Walk 记录' : '2026 年 5 月'" back-route="diary" @go="go" />
        <div class="tabs">
          <button type="button" class="active">流水账</button>
          <button type="button" @click="go(route === 'diary-log-color' ? 'diary-essay-color' : 'diary-essay')">小作文</button>
        </div>
        <article class="essay-card">
          <h2>{{ route === "diary-log-color" ? "绿色收集记录" : "今天的小天气" }}</h2>
          <p>{{ route === "diary-log-color" ? "按顺序记录路上遇到的颜色。" : "按时间记录传感器看到的变化。" }}</p>
          <div class="log-list">
            <div class="log-item"><strong>15:02 光照变亮</strong><p>lux 从 86 上升到 520，小芽判断我们走到室外。</p></div>
            <div class="log-item"><strong>15:08 听见城市声音</strong><p>声音波动变大，可能经过了路口和树下。</p></div>
            <div class="log-item"><strong>15:18 拍到 5 种绿色</strong><p>银杏、草丛、店门口的小盆栽、路牌和树影。</p></div>
          </div>
        </article>
        <BottomNav active="diary" @go="go" @random-invite="randomInvite" />
      </section>

      <section v-else-if="route === 'diary-essay-color'" class="screen scroll">
        <div class="long-page">
          <PhoneHeader title="小芽日记" subtitle="Color Walk 小作文" back-route="diary" @go="go" />
          <div class="tabs">
            <button type="button" @click="go('diary-log')">流水账</button>
            <button type="button" class="active">小作文</button>
          </div>
          <article class="essay-card color-essay">
            <h2>今天我收集了很多绿色</h2>
            <p>我们从房间里出发，先遇见了树影，然后遇见了草丛、路牌和店门口的小盆栽。</p>
            <p>这些绿色不是一种颜色。它们有的很亮，有的偏黄，有的藏在阴影里。小芽把它们都记住了。</p>
            <p>如果以后我又有点蔫了，可以翻回这一天，看见你带我去过真实的街道。</p>
            <button type="button" class="button primary share-long" @click="go('share')">分享</button>
          </article>
          <BottomNav active="diary" @go="go" @random-invite="randomInvite" />
        </div>
      </section>

      <section v-else-if="route === 'diary-essay'" class="screen">
        <PhoneHeader title="小芽日记" subtitle="今日散步小作文" back-route="diary" @go="go" />
        <div class="tabs">
          <button type="button" @click="go('diary-log')">流水账</button>
          <button type="button" class="active">小作文</button>
        </div>
        <article class="essay-card">
          <h2>今天我听见了春天</h2>
          <p>今天我们走到了有树影的路边。阳光先落在我的叶子上，然后慢慢变暖。</p>
          <p>我听见了车轮、脚步和风。你拍下的绿色像新的叶尖，提醒我真实世界一直在悄悄长大。</p>
          <div class="button-row essay-actions"><button type="button" class="button primary" @click="go('share')">分享</button></div>
        </article>
        <BottomNav active="diary" @go="go" @random-invite="randomInvite" />
      </section>

      <section v-else-if="route === 'map'" class="screen">
        <PhoneHeader title="散步地图" subtitle="这个月和小芽走过的地方" />
        <article class="essay-card map-large">
          <h2>国权路 · 树影路线</h2>
          <p>32 min walk · 3 sounds · 5 greens</p>
          <div class="mini-map map-expanded"></div>
        </article>
        <BottomNav active="diary" @go="go" @random-invite="randomInvite" />
      </section>

      <section v-else-if="route === 'nearby'" class="screen">
        <PhoneHeader title="附近" subtitle="城市里新长出来的线索" />
        <article class="prompt-card local">
          <p class="prompt-label">Local Discovery</p>
          <h2 class="prompt-title small">国权路的咖啡店换了新菜单。如果我们走路过去，还能看看路边的银杏。</h2>
        </article>
        <Sprout />
        <BottomNav active="nearby" @go="go" @random-invite="randomInvite" />
      </section>

      <section v-else-if="route === 'me'" class="screen">
        <PhoneHeader title="我的" />
        <article class="profile-card">
          <Sprout mood="happy" variant="small-profile" />
          <div class="profile-text">
            <h2>Chiara 的小芽</h2>
            <p>成长 18 天 · 今日很想见光</p>
            <p>已一起散步 12 次</p>
            <p>成长值：65</p>
            <div class="growth"><span></span></div>
          </div>
        </article>
        <article class="settings-card">
          <div v-for="row in ['设备状态', '语音音色', '提醒时间', '数据权限']" :key="row" class="setting-row"><span>{{ row }}</span><span>›</span></div>
        </article>
        <article class="atlas-entry" @click="go('atlas')">
          <h2>我解锁的小芽 <span>›</span></h2>
          <div class="badges"><span class="badge"></span><span class="badge"></span><span class="badge"></span><span class="badge"></span></div>
        </article>
        <BottomNav active="me" @go="go" @random-invite="randomInvite" />
      </section>

      <section v-else-if="route === 'atlas'" class="screen">
        <PhoneHeader title="小芽图鉴" subtitle="收集不同的小芽形态" back-route="me" @go="go" />
        <div class="atlas-grid">
          <article v-for="item in ['Light Sprout', 'Color Sprout', 'City Sprout', 'Sound Sprout', 'Night Sprout', 'Bloom Sprout']" :key="item" class="atlas-card">
            <div class="tiny-sprout"><span class="leaf-a"></span><span class="leaf-b"></span><span class="stem"></span><span class="pot"></span></div>
            <h3>{{ item }}</h3>
            <p>{{ item.includes('Night') || item.includes('Bloom') ? '去散步解锁' : '已解锁' }}</p>
          </article>
        </div>
        <BottomNav active="me" @go="go" @random-invite="randomInvite" />
      </section>

      <section v-else-if="route === 'share'" class="screen">
        <PhoneHeader title="分享卡片" subtitle="把今天的小芽日记发给朋友" back-route="diary-essay" @go="go" />
        <article class="share-card">
          <div class="share-hero"><Sprout mood="happy" variant="share-mini" /><span class="share-sun"></span><span class="share-tree"></span></div>
          <h2>今天我听见了春天</h2>
          <p>32 min walk · 3 sounds · 5 greens</p>
          <p class="share-copy">刚才的光不是屏幕里的光。它落在我的叶子上，也落在你的鞋尖前面。</p>
          <span class="qr" aria-hidden="true"></span>
        </article>
        <div class="button-row share-actions">
          <button type="button" class="button primary wide" @click="saveShare">保存图片</button>
          <button type="button" class="button secondary wide" @click="openShare">分享</button>
        </div>
        <article class="privacy-card"><h3>隐私提示</h3><p>分享卡片默认不包含精确位置。</p></article>
        <div v-if="toast" class="toast">{{ toast }}</div>
        <div v-if="shareSheet" class="share-sheet">
          <h3>分享到</h3>
          <div class="share-options"><button>小红书</button><button>微信</button><button>朋友圈</button></div>
        </div>
      </section>
    </section>

    <aside class="demo-notes">
      <p class="kicker">Vue Version</p>
      <h1>出走小芽 Vue Demo</h1>
      <p>这是从最新组件化 HTML 版迁移出来的 Vue/Vite 版本，方便继续拆组件、加动效、接 Flask 或云端后端。</p>
      <div class="demo-actions">
        <button type="button" @click="go('home')">首页</button>
        <button type="button" @click="randomInvite">随机散步</button>
        <button type="button" @click="go('diary')">日记</button>
        <button type="button" @click="go('me')">我的</button>
      </div>
    </aside>
  </main>
</template>
