export const navIcons = {
  home: {
    selected: "icon for front page-sclected.svg",
    normal: "icon for front page-not sclected.svg",
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

export const invites = {
  light: {
    type: "light",
    label: "小芽想出门：晒太阳散步",
    title: "带我去楼下晒3分钟太阳好吗？",
    subtitle: "一个很小的照顾任务",
    walkTitle: "晒太阳散步",
    walkSubtitle: "出门晒太阳咯",
    speech: ["你知道公园二十分钟定律吗？", "出门晒晒太阳，一会儿我们都能满血复活。"],
    sprout: "sprout-wilted.svg",
  },
  sound: {
    type: "sound",
    label: "小芽想出门：听城市声音",
    title: "今天去听听城市里的声音，好吗？",
    subtitle: "一个很小的照顾任务",
    walkTitle: "听城市声音",
    walkSubtitle: "听听真实世界的声音",
    speech: ["我听见了风、脚步和远处的车声。", "城市今天不像房间里那么安静。"],
    graphic: "sound-graphic.svg",
    sprout: "sprout-happy.svg",
  },
  color: {
    type: "color",
    label: "小芽想出门：收集绿色",
    title: "我们去周围找2种绿色的东西，怎么样？",
    subtitle: "一个很小的照顾任务",
    walkTitle: "收集绿色",
    walkSubtitle: "收集今天路上的绿色",
    speech: ["路边的颜色比屏幕里柔软很多。", "我想把这些绿色记进今天的日记。"],
    sprout: "sprout-happy.svg",
  },
  local: {
    type: "local",
    label: "小芽想出门：附近走走",
    title: "我们去附近走走，看看路边的树影吧！",
    subtitle: "一个很小的照顾任务",
    walkTitle: "附近走走",
    walkSubtitle: "看看附近新长出的线索",
    speech: ["附近也有很多新的东西在长出来。", "我们可以慢慢走过去看一看。"],
    sprout: "sprout-happy.svg",
    local: true,
  },
};

/** Demo 主路径仅开放 light / color */
export const demoInviteTypes = ["light", "color"];

export const inviteTypes = Object.keys(invites);

export const atlasItems = [
  ["Sunny Sprout", "atlas-Sunny Sprout.svg", "已解锁"],
  ["Sound Sprout", "atlas-Sound Sprout.svg", "已解锁"],
  ["City Sprout", "atlas-City Sprout.svg", "已解锁"],
  ["Rain Sprout", "atlas-Rain Sprout.svg", "已解锁"],
  ["Night Sprout", "atlas-Night Sprout.svg", "去散步解锁"],
  ["Bloom Sprout", "atlas-Bloom Sprout.svg", "去散步解锁"],
];

export const LEGACY_ROUTE_MAP = {
  home: "/home",
  "invite-light": "/invite/light",
  "invite-sound": "/invite/sound",
  "invite-color": "/invite/color",
  "invite-local": "/invite/local",
  "walk-light": "/walk/light",
  "walk-sound": "/walk/sound",
  "walk-color": "/walk/color",
  "walk-local": "/walk/local",
  "pause-light": "/pause/light",
  "pause-sound": "/pause/sound",
  "pause-color": "/pause/color",
  "pause-local": "/pause/local",
  finish: "/finish",
  "finish-color": "/finish/color",
  diary: "/diary",
  "diary-log": "/diary/log",
  "diary-log-color": "/diary/log/color",
  "diary-essay": "/diary/essay",
  "diary-essay-color": "/diary/essay/color",
  map: "/map",
  nearby: "/nearby",
  me: "/me",
  atlas: "/atlas",
  share: "/share",
};
