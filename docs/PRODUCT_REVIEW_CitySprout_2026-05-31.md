# 出走小芽 City Sprout — 完整产品评审报告

**版本**：2026-05-31  
**评审依据**：

- [PRD_CitySprout_软硬件一体版_2026-05-28.md](../PRD_CitySprout_软硬件一体版_2026-05-28.md)
- [VERIFICATION_CitySprout.md](./VERIFICATION_CitySprout.md)
- 后端：[`backend/`](../backend/)（Flask、`/plant`、`/walk/*`、AI 模块）
- 前端：[`app_demo/08_vue_figma_strict_demo/`](../app_demo/08_vue_figma_strict_demo/)（Vue 3 + Figma 严格 Demo）
- 固件：[`arduino/city_sprout_pahub_main_v4_no_flicker_canvas/`](../arduino/city_sprout_pahub_main_v4_no_flicker_canvas/)

**评审视角**：消费级 AI 硬件 × 内容社区 × 年轻用户 × 产品增长（小红书 PM 视角）  
**评审原则**：严格、克制、可执行；不堆功能；AI 做人格化表达，规则做任务判定；所有建议尽量精确到场景、页面、字段、接口、验收标准。

---

## 目录

1. [执行摘要](#1-执行摘要)
2. [当前产品一句话概括](#2-当前产品一句话概括)
3. [推荐产品定位](#3-推荐产品定位)
4. [核心用户画像](#4-核心用户画像)
5. [用户需求真实性评估](#5-用户需求真实性评估)
6. [硬件必要性评估](#6-硬件必要性评估)
7. [AI 必要性评估](#7-ai-必要性评估)
8. [当前用户旅程](#8-当前用户旅程)
9. [推荐用户旅程](#9-推荐用户旅程)
10. [页面信息架构修改建议](#10-页面信息架构修改建议)
11. [状态机修改建议](#11-状态机修改建议)
12. [小红书传播潜力](#12-小红书传播潜力)
13. [MVP 功能优先级](#13-mvp-功能优先级)
14. [精确修改清单（问题 + 解决方案）](#14-精确修改清单问题--解决方案)
15. [V0.1 / V0.2 / V0.3 版本规划](#15-v01--v02--v03-版本规划)
16. [代码和 PRD 差异](#16-代码和-prd-差异)
17. [团队分工建议](#17-团队分工建议)
18. [Demo 验收方案](#18-demo-验收方案)
19. [待确认问题](#19-待确认问题)
20. [附录：仓库实现进度快照](#20-附录仓库实现进度快照)

---

## 1. 执行摘要

### 1.1 产品是否成立

**方向成立，Demo 形态尚未成立。**

「一棵想见真实世界、会写散步日记的 AI 小植物」对应的真实需求存在：

- 低门槛离开室内（不是运动 KPI）
- 实体陪伴感（不是手机提醒）
- 轻量任务 + 可分享记录（不是复杂养成游戏）

PRD 第 3 节定义的闭环在代码里**链路已基本打通**：

```text
硬件 POST /plant → AI 文案 + TTS
       ↓
GET /latest → Vue 首页同步
       ↓
/walk/start → photo/audio（可选）→ /walk/diary
       ↓
日记 / 图鉴 / 地图 ← /latest
```

但用户实际感知是：**仪表盘 + 四种英文 Walk 小游戏 + 大量静态假页面**，而不是「一棵想出门的小芽」。

### 1.2 最有潜力的价值

| 层级 | 价值 | 当前状态 |
|------|------|---------|
| 硬件 | 双屏人格（彩屏动画 + OLED 短句 + 可选 TTS）+ 光照/移动感知 | 已实现，Demo 稳定性受 WiFi/麦克风/TTS 影响 |
| AI | 实时状态文案 + 散步结束日记 | 文案链路完整；日记与传感器弱绑定；Sound 为 stub |
| 传播 | 散步完成后的分享卡片 | **未实现**（Share 页为静态假数据） |
| 留存 | 第 N 次出走日记序列 | 后端曾仅内存；持久化方案已部分起草 |

### 1.3 最大问题（按严重度）

| # | 问题 | 用户影响 | 严重度 |
|---|------|---------|--------|
| 1 | **定位过散**：Light/Sound/Color/Local 并列，底栏「散步」曾随机跳转 | 30 秒内不知道产品是什么 | 高 |
| 2 | **软硬件任务脱节**：Walk 在手机端完成，固件不知 walk session | 室内可「完成散步」，硬件像摆设 | 高 |
| 3 | **分享闭环断裂**：ShareScreen 静态假文案，无法导出 PNG | 无法发小红书，做完无奖励感 | 高 |
| 4 | **AI 信任感不足**：Sound 音频 stub；日记 events 常空 | 用户认为 AI 在「瞎写」 | 中高 |
| 5 | **首页像仪表盘**：默认展示 lux/温湿度等 4 行传感器 | 不知道下一步做什么 | 高 |
| 6 | **Demo 稳定性**：HTTP -1、Mic peak=0、TTS 阻塞 loop | 演示翻车 | 高 |

### 1.4 当前最需要做的减法

**立即从首次体验路径移除或隐藏：**

- Nearby（静态假社区）
- Local Discovery（预设探店，非真实联动）
- Sound Walk Demo 入口（音频 AI 为 stub，伤害信任）
- 地图轨迹（静态 SVG）
- 首页传感器 raw 字段（移入调试面板）

**立即聚焦：**

> **情绪陪伴型 Light Walk** — 5–10 分钟户外晒光 → 硬件反馈 → AI 日记 → **可导出分享卡**

### 1.5 第一版应聚焦什么

一句话：**让用户在 10 分钟内完成「带小芽晒太阳 → 得到一篇真实日记 → 保存一张分享图」**。

---

## 2. 当前产品一句话概括

### 在用户眼中（评审时）

> **「一个会显示 lux/温湿度的智能硬件仪表盘 + 四种英文散步小游戏 + 日记/图鉴/附近内容的 Figma 原型 App」**

### 应该成为

> **「一棵会撒娇、会写日记的 AI 小植物，帮你用几分钟离开房间，看看真实的阳光」**

---

## 3. 推荐产品定位

### 3.1 当前定位存在的问题

| 问题类型 | 具体表现 | 代码/文档证据 |
|---------|---------|--------------|
| 定位太散 | 陪伴、运动、探索、疗愈、养成、UGC 平均用力 | PRD 多章节并列；App 15 屏 |
| 理解成本高 | `Light Walk` / `Color Walk` 英文标签 | `constants/demo.js` invites |
| 硬件价值不足 | 任务完成不依赖携带与感知 | 无 walk 完成校验（评审基线） |
| AI 价值不足 | 分享卡假、Sound stub | `ShareScreen.vue`、`ai/audio.py` |
| 难以传播 | 无 PNG 导出 | `useShareSheet.js` 仅 Toast |

### 3.2 三个定位方案

#### 方案 A：情绪陪伴型（**推荐 Demo**）

> 一棵会撒娇、会写日记的桌面小芽，帮你用 5 分钟离开房间晒晒太阳。

| 维度 | 评估 |
|------|------|
| 优点 | 硬件 OLED/语音/动画价值最大；任务门槛最低；小红书故事线清晰 |
| 缺点 | 长期留存依赖日记序列与成长视觉 |
| 适合 | 首次 Demo、校园/工位场景、硬件展示 |

#### 方案 B：户外探索型

> 带小芽收集城市里的光、声、色，每次出门生成一篇探索日记。

| 维度 | 评估 |
|------|------|
| 优点 | 内容差异化强，适合 Color/Sound Walk |
| 缺点 | 4 任务 + 拍照/录音，首次 10 分钟内难完成 |
| 适合 | V0.2 以后 |

#### 方案 C：轻量养成型

> 通过每日带小芽出门，解锁不同形态小芽图鉴。

| 维度 | 评估 |
|------|------|
| 优点 | 留存机制明确 |
| 缺点 | 图鉴曾 4/6 默认解锁，养成感虚假 |
| 适合 | 有持久化 + 真实解锁逻辑后 |

### 3.3 推荐方案

**选 A**。Color Walk 作为 P1 增强；Sound/Local/Nearby 延后或 Demo 隐藏。

### 3.4 用户为什么愿意使用

| 维度 | 回答 |
|------|------|
| 第一驱动力 | 可爱硬件 + 「它想晒太阳」的无 KPI 压力邀请 |
| 连续 7 天 | 第 N 次出走日记、小芽蔫→精神视觉变化、分享卡差异 |
| 愿意分享 | 硬件实拍 + AI 金句 + 「今天晒了 X 分钟太阳」 |
| 可能流失 | 配网失败、不知何时算完成、AI 雷同、像仪表盘 |
| 最小长期价值 | **「小芽出走日记本」** — 可回看、可分享的拟人记录 |

---

## 4. 核心用户画像

### 4.1 用户类型优先级

| 用户类型 | 第一版 | 说明 |
|---------|--------|------|
| 久坐宿舍/工位学生、年轻白领 | **核心** | 需要被可爱事物邀请出门 |
| 喜欢桌面摆件/可爱硬件 | 次级 | 购买动机强，需软件闭环 |
| 喜欢记录生活/发小红书 | 次级 | 需真实分享卡 |
| 喜欢养成游戏 | 不建议优先 | 图鉴系统未就绪 |
| 喜欢 AI 玩具尝鲜 | 不建议优先 | 无日记/分享则一次性流失 |

### 4.2 关键洞察

| 维度 | 内容 |
|------|------|
| 最强痛点 | 知道该出门，但缺「不羞耻、不沉重」的触发 |
| 最易打动卖点 | 「小芽蔫了，晒 3 分钟就好」+ 设备真的会说话/变表情 |
| 最易放弃原因 | 流程不清、联调失败、做完没有可晒的东西 |
| 发小红书理由 | 反差萌、治愈卡片、校园/工位实拍 |
| 不愿分享原因 | 分享卡是假的、隐私顾虑、日记像模板 |

---

## 5. 用户需求真实性评估

### 5.1 真实需求（成立）

| 需求 | 是否真实 | 说明 |
|------|---------|------|
| 低门槛情绪触发 | ✓ | 比运动 App、手环更软 |
| 实体陪伴 | ✓ | 桌面小芽 + 外出携带 |
| 轻量户外 | ✓ | 3–10 分钟晒光即可 |
| 可分享记录 | ✓ | 小红书 UGC 天然场景 |

### 5.2 伪需求 / 高风险需求

| 需求 | 判定 | 理由 | 建议 |
|------|------|------|------|
| Nearby 附近新店 | **伪（Demo 版）** | 全静态，无 API | P3 删除路径 |
| 四种 Walk 并列必选 | **伪选择** | 首次用户决策负担 | 默认 Light，Color 为 P1 |
| ENV/IAQ 驱动任务 | **伪** | 用户无感知 | 不进首页，仅 AI 素材 |
| 硬件实时声音识别 | **延后** | PRD 第 20 节已明确 | 隐藏 Sound Walk |
| 复杂养成/社区 | **延后** | 无持久化时不成立 | V0.2+ |

### 5.3 与替代品差异

| 对比 | 小芽差异 | 成立条件 |
|------|---------|---------|
| 手机提醒 | 实体眼神、OLED、语音 | 硬件反馈稳定 |
| 运动手环 | 无 KPI、有情绪叙事 | 不说「步数」 |
| 桌面摆件 | 感知世界 + 写日记 | 软件闭环跑通 |
| 普通 Chatbot | 传感器绑定 + 外出仪式 | 日记与 lux/移动相关 |

---

## 6. 硬件必要性评估

### 6.1 模块对照表（以仓库实际为准）

| 硬件模块 | 用户感知价值 | MVP 必须 | 当前问题 | 解决方案 |
|----------|-------------|---------|---------|---------|
| AtomS3R 彩屏 Canvas | 高：表情/摇摆/开花 | **P0** | MP3 播放时 `loop()` return，传感暂停 | 播放 MP3 时不 return 整个 loop，仅调用 `serviceMp3Playback()` |
| PaHUB + DLight BH1750 | 高：晒太阳叙事核心 | **P0** | 阈值未标定，室内强光误判 | Demo 前标定 `LUX_WILTED_MAX=50`、`LUX_NEED_SUN_MAX=300` |
| IMU（S3R 内置） | 高：「正在散步」 | **P0** | 手持抖动即 walking | 与 walk session + lux 联合校验完成 |
| OLED SH1107 | 高：不看手机知状态 | **P0** | ~20s 更新；中文被 `toOLEDAscii` 剥离 | 状态变化 1s 内刷新；短句用拼音/英文关键词或保留常用汉字 |
| Voice Base 扬声器 | 中：陪伴感 | **P1** | 无 Key 时英文库 | 预置中文 MP3 fallback |
| Voice Base 麦克风 | 低 | **P2** | peak=0 常见；不稳定 | 不参与任务判定；仅 place 辅助 |
| ENV Pro BME68x | 低 | **P2** | I2C 复杂度 | 仅传入 diary prompt，不进首页 |
| Wi-Fi | 高 | **P0** | `127.0.0.1`、IP 不一致 → HTTP -1 | 启动脚本打印局域网 IP；`arduino_secrets.h` 验收清单 |
| **Atom-Matrix 5×5 LED** | **无** | **P3** | **背景表有，代码不存在** | 对外文档删除或标注「未接入」 |

> **说明**：产品背景表中的 Atom-Matrix v1.1（5×5 LED + IMU）在当前主固件中**未接入**。实际使用 S3R 内置 IMU + 彩屏 Canvas 代替 LED 矩阵表达。

### 6.2 为什么一定要有硬件

| 问题 | 结论 |
|------|------|
| 手机 App 能否完成同样功能？ | **功能上可以**，但失去实体陪伴与「带它出门」仪式 |
| 用户为什么要把小芽带出去？ | 叙事要求 + 光照/移动必须由设备感知，否则像纯 App |
| 硬件如何建立陪伴感？ | 彩屏眼神、OLED 短句、可选 TTS、外出时状态变化 |
| 哪些反馈增强情绪价值？ | 蔫→摇→开花；「晒到太阳啦」OLED + 动画 |
| 哪些硬件增加复杂度但用户无感？ | ENV 首页展示、麦克风任务判定、Matrix LED（未接） |

### 6.3 硬件与任务协同方案

**问题**：散步 session 仅在 Vue/Flask，固件每 20s POST `/plant`，双方不知彼此任务状态。

**解决方案**：

1. **规则层（后端）**：`POST /walk/diary` 前校验 `lux ≥ 300` 累计 ≥ 180s 且 `motion` 曾为 active（或 `SPROUT_DEMO_BYPASS_WALK=1` 调试）
2. **硬件层（固件）**：`/plant` 上报 lux/motion；状态变 `sunlight` 时 OLED 立即刷新
3. **App 层**：Light Walk 页展示「晒光进度 X/180 秒」，达标后高亮「可以完成啦」
4. **plant.py**：有 active light walk 时，每次 `/plant` 调用 `tick_active_walk_from_plant(lux, motion)`

---

## 7. AI 必要性评估

### 7.1 不应交给 AI 的部分

| 职责 | 负责模块 | 原因 |
|------|---------|------|
| 是否走出室内 / 晒够太阳 | 规则：lux + 时长 | 需确定性、可验收 |
| 是否达到移动量 | 规则：IMU motion | 同上 |
| 任务是否完成 | 规则：`walk_completion_allowed()` | 不能靠 LLM 判断 |
| 是否重复生成 diary | 规则：walk_id 单次 finish | 防重复提交 |
| 状态机切换 | 固件 + 阈值 | 稳定、可调试 |

### 7.2 适合交给 AI 的部分

| 职责 | 模块 | 输出 |
|------|------|------|
| 小芽人格化表达 | `ai/speech.py` + `prompts/sprout_speech.md` | `speech_short`, `speech_full` |
| 散步日记 | `ai/diary.py` | `title`, `essay`, `timeline`, `map_summary` |
| 照片理解 | `ai/vision.py` | objects, colors, summary |
| 分享文案 | diary 扩展 | `share_caption` |
| 城市观察元素 | diary 扩展 | `discovered_elements` |

### 7.3 当前 AI 问题与解决方案

| # | 问题 | 现状 | 解决方案 |
|---|------|------|---------|
| 1 | Sound Walk 假分析 | `ai/audio.py` 不读文件，随机/猜 | Demo **隐藏** Sound 入口；V0.2 再接真实 ASR/分类 |
| 2 | 日记与传感器脱节 | `diary.py` 仅 session events，常为空 | prompt 注入 `latest` 的 lux/motion/place/温湿度 + walk 时长 + light_progress |
| 3 | 无 share 字段 | diary 无 share_caption | 扩展 JSON 输出；fallback 规则生成 |
| 4 | API 失败无感知 | 有 speech fallback，diary 有模板 | 完成页标注「小芽用了备用日记」可选；保证中文 |
| 5 | 文案风格不一致 | speech 与 diary 分开调 | 统一 system：温柔、短句、第一人称、不命令 |
| 6 | 日记过长 | 无硬限制 | prompt 限制 essay 120–180 字；OLED 只用 speech_short ≤18 字 |

### 7.4 推荐 AI 输出结构（V0.1）

```json
{
  "speech_short": "晒到太阳啦",
  "speech_full": "今天的风有点甜，光落在叶子上暖融融的。",
  "diary_title": "第一次看见窗外的天空",
  "diary_full": "今天主人带我走出了房间……",
  "share_caption": "今天带小芽晒太阳，它写了一篇日记。",
  "discovered_elements": ["阳光", "树影", "微风"],
  "growth_delta": 12
}
```

| 字段 | 必须有 | 规则生成 | AI 生成 | 用户可编辑 |
|------|--------|---------|---------|-----------|
| speech_short/full | ✓ | fallback | ✓ | 否 |
| diary_title/essay | ✓ | fallback | ✓ | 否 |
| share_caption | ✓ | 可 | ✓ | **是** |
| growth_delta | 可选 | ✓ (+12/次) | 否 | 否 |
| discovered_elements | 可选 | 部分 | ✓ | 否 |

### 7.5 推荐 Prompt 结构（diary，暂不改代码，供讨论）

**System**：

```text
你是 City Sprout「出走小芽」，一棵可携带的 AI 小植物。
用第一人称、温柔、有画面感的中文写散步日记。
不要命令用户，不要健康打卡口吻。
essay 控制在 120–180 字。
输出合法 JSON。
```

**User 输入应包含**：

- walk_type、started_at、duration_seconds
- light_progress（accumulated_seconds / complete）
- latest snapshot：state, lux, motion, place, temperature_c, humidity_percent
- photos[].summary, colors, objects
- events[]（若有）

**Fallback 策略**：按 walk_type + lux 档位选模板；注入真实 `sunlight_seconds` 与 photo_count 替换占位。

---

## 8. 当前用户旅程

### 8.1 逐步评估表

| 步骤 | 当前体验 | 用户是否理解 | 问题 | 严重度 | 修改建议 |
|------|---------|-------------|------|--------|---------|
| 首次看到硬件 | 彩屏小芽 + OLED 英文状态行 | 部分 | 不知要配网/打开网页 | 中 | 包装卡 3 步：WiFi → 打开 URL → 点「晒太阳」 |
| 开机/配网 | 改 `arduino_secrets.h` | 否 | 技术门槛高 | **高** | 预配热点 + QR；启动脚本打印 IP |
| 进入页面 | `#/home` 曾展示 4 行传感器 | 否 | 像仪表盘 | **高** | 仅：状态标题 + speech + 主按钮 |
| 查看状态 | state 中文标题（有点蔫了等） | 是 | — | 低 | — |
| 接收任务 | speech 卡 smart / 底栏曾 random | 混乱 | 两入口不一致 | **高** | 统一 `smartWalkInvite`，默认 light |
| 接受任务 | 邀请页 → 出门 → `/walk/start` | 部分 | 英文 Walk 名 | 中 | 中文：「晒太阳散步」 |
| 带小芽出门 | 无完成标准 | 否 | 室内可完成 | **高** | lux≥300 累计 180s |
| 散步中 | 4 种 UI；Light 几乎无互动 | 部分 | 不知进度 | 中 | 计时 + 晒光进度条 |
| 完成任务 | 暂停页手动「完成散步」 | 部分 | 无「晒够了」反馈 | 中 | 达标后 Toast + 按钮高亮 |
| 上传照片 | Color 专用，曾需 5 张 | — | 步骤重 | 中 | Light 可选 1 张；Color 改为 2 张 |
| AI 日记 | `POST /walk/diary` | 部分 | 等待无进度 | 中 | 「小芽正在写日记…」 |
| 查看结果 | finish + diary 页 | 是 | 可能模板化 | 中 | 注入 sensor；展示 growth + 第 N 次 |
| 分享 | Share 静态假数据 | **否** | **无法发小红书** | **高** | html2canvas 导出 PNG |
| 再次使用 | 无历史/计数展示 | 否 | 无留存动机 | 中 | walk_count + 昨日日记入口 |

### 8.2 首次体验耗时（评审基线）

| 环节 | 耗时 |
|------|------|
| 配网 + 联调 | ~5 min |
| 理解首页 | ~2 min |
| 选择 Walk 类型 | ~1 min |
| 户外散步 | ~3 min |
| Color 拍 5 张 | ~5 min+ |
| 日记生成 | ~1 min |
| **合计** | **~17 min+**（超出 5–10 min 目标） |

---

## 9. 推荐用户旅程

### 9.1 目标路径（5–10 分钟）

```text
开机 OLED：小芽醒了 / 想晒太阳
    ↓
手机首页：有点蔫了 + speech + 【出门晒太阳（约 3 分钟）】
    ↓
一键 start light walk → 散步中（计时 + 晒光进度）
    ↓
lux ≥ 300 累计 180s → 硬件 sunlight + OLED「晒到太阳啦」
    ↓
可选拍 1 张照片
    ↓
暂停页 → 完成散步 → AI 日记（15s 内，有 loading）
    ↓
完成页 → 【生成分享卡】→ 编辑 caption → 保存 PNG
    ↓
次日：首页「第 2 次出走」+ 历史日记入口
```

### 9.2 各环节规格

| 环节 | OLED | 彩屏 | 页面 | AI |
|------|------|------|------|-----|
| 邀请 | 想晒太阳 | 轻摇 | 主按钮高亮 | speech_full |
| 散步中 | 在路上 | 摇摆 | 进度 X/180s | 否 |
| 晒够 | 晒到太阳啦 | 开花 | 提示可完成 | 短句 optional |
| 写日记 | 写日记中 | 思考 | loading | diary |
| 完成 | 今天很好 | happy | essay + 分享 | 已完成 |

### 9.3 散步中是否频繁看手机

**不需要。** 以硬件 OLED/彩屏/TTS 为主；手机仅偶尔看进度条。

---

## 10. 页面信息架构修改建议

### 10.1 PRD 首页三问对照

| 问题 | 当前 | 目标 |
|------|------|------|
| 小芽现在怎么样？ | 有 state 标题 | 保留 + 插画随 state 变 |
| 小芽刚刚说了什么？ | speech 卡 | 保留 |
| 我现在可以带它做什么？ | **弱** | **主按钮「出门晒太阳」** |

### 10.2 页面清单

| 页面 | 路由 | 当前功能 | 必须保留 | 删除/隐藏 | 新增 | 优先级 |
|------|------|---------|---------|----------|------|--------|
| 首页 | `/home` | 状态+传感器+speech | 状态、speech、CTA | 传感器 raw | 主按钮 | **P0** |
| 邀请 | `/invite/:type` | 4 种邀请 | light/color | sound/local 路由 | 中文文案 | **P0** |
| 散步中 | `/walk/:type` | 4 种 UI | Light 计时+进度 | Sound UI | 晒光进度条 | **P0** |
| 暂停 | `/pause/:type` | 继续/完成 | 是 | — | 未达标提示 | **P0** |
| 完成 | `/finish` | essay 展示 | 是 | — | 分享 CTA | **P0** |
| 日记 | `/diary/*` | 流水账+小作文 | essay、照片 | 假 calendar 统计 | 绑定 walk_diary | **P0** |
| 分享 | `/share` | **静态假数据** | 框架 | 假字段 | PNG 导出 | **P0** |
| 历史 | `/diary` hub | 静态 May 2026 | — | 假数据 | diary_history | **P1** |
| 附近 | `/nearby` | 静态 6 卡 | — | **整页** | — | **P3** |
| 地图 | `/map` | 静态 SVG | — | Demo 隐藏 | — | **P2** |
| 图鉴 | `/atlas` | 6 项 | 二级入口 | 默认全解锁 | 真实解锁 | **P2** |
| 我的 | `/me` | lux/motion raw | — | 设备 raw | 调试开关 | **P2** |

### 10.3 推荐 V0.1 仅 4 个用户页 + 调试

```text
1. 首页：小芽状态 + 当前任务 + 主按钮
2. 散步中：简单进度 + 小芽反馈
3. 日记详情：AI 日记 + 照片 + 分享入口
4. 分享：可编辑 caption + 保存 PNG
（设置/调试：DemoNotesPanel，不对普通用户展示）
```

---

## 11. 状态机修改建议

### 11.1 用户可见状态 ↔ 底层状态

| 用户可见状态 | 底层 state | 触发条件 | OLED 文案 | 页面 |
|-------------|-----------|---------|----------|------|
| 刚刚醒来 | idle | 开机 | 小芽醒了 | 有点安静 |
| 有点蔫 | wilted | lux < 50 | 有点没精神 | 有点蔫了 + CTA |
| 想出门 | need_sun | lux < 300 | 想晒太阳 | 主按钮高亮 |
| 正在探险 | walking | motion active | 在路上 | 散步计时 |
| 晒到太阳啦 | sunlight | lux ≥ 300 | 晒到太阳啦 | 进度完成 |
| 正在写日记 | — | POST diary | 写日记中 | loading |
| 今天很开心 | — | diary ready | 今天很好 | 完成页 |

### 11.2 固件当前状态（已实现）

```text
PlantState: idle | wilted | need_sun | sunlight | walking
MotionState: still | active
SoundState: quiet | active | intense | unknown
PlaceState: indoor | outside | unknown
```

文件：[`city_sprout_pahub_main_v4_no_flicker_canvas.ino`](../arduino/city_sprout_pahub_main_v4_no_flicker_canvas/city_sprout_pahub_main_v4_no_flicker_canvas.ino)

### 11.3 状态机问题与解决方案

| 问题 | 解决方案 |
|------|---------|
| OLED 显示 `NEED SUN` 等英文 | 改为中文关键词或拼音；与 App 中文 state 对齐 |
| 状态变后 OLED 延迟 ~20s | 检测 `currentState` 变化 → 立即 `showTextOnOLED(true)` |
| 用户看到 lux/IMU 原始值 | 首页/App 隐藏；仅 `DemoNotesPanel` 展示 |
| 后端无状态机 | **保持**；后端只消费 `/plant` 的 state 字段，不重复计算 |
| walking 与 sunlight 优先级 | motion 时强制 walking；高 lux 且无 motion 为 sunlight |

---

## 12. 小红书传播潜力

### 12.1 最适合 3 个内容场景

1. **第一次带小芽晒太阳** — 「我的 AI 植物蔫了催我出门」
2. **小芽写的意外可爱日记** — 截取 essay 金句
3. **连续 7 天出走** — 状态变化拼图 + 「第 7 次出走」

### 12.2 分享卡片结构（精确字段）

```text
┌─────────────────────────────────┐
│  出走小芽 City Sprout            │
│  小芽的第 {walk_number} 次出走    │
│  今天晒了 {sunlight_seconds} 秒太阳 │
│  ─────────────────────────────  │
│  {essay_excerpt 1-2 句}          │
│  发现：{discovered_elements}      │
│  成长 +{growth_delta}             │
│  [用户照片，可选]                 │
│  #{share_caption 可编辑}         │
│  （不含精确位置）                 │
└─────────────────────────────────┘
```

| 字段 | 必须 | 来源 | 隐私 |
|------|------|------|------|
| walk_number | ✓ | 后端 `_walk_count` | 低 |
| sunlight_seconds | ✓ | light_progress | 低 |
| essay_excerpt | ✓ | AI essay 截取 | 低 |
| 小芽插画 | ✓ | state → SVG | 低 |
| 用户照片 | 推荐 | walk photos | 用户选 |
| growth_delta | 可选 | 规则 +12 | 低 |
| GPS/精确地址 | **禁止** | — | **高** |

### 12.3 分享路径

```text
完成散步 → POST /walk/diary
    → 完成页预览摘要
    → /share 渲染卡片（绑定 walk_diary）
    → 用户编辑 share_caption
    → html2canvas → 下载 PNG
    → 小红书 / 朋友圈
```

### 12.4 当前缺口

- [`ShareScreen.vue`](../app_demo/08_vue_figma_strict_demo/src/views/ShareScreen.vue)：写死「今天我听见了春天」「32 min walk · 3 sounds · 5 greens」
- [`useShareSheet.js`](../app_demo/08_vue_figma_strict_demo/src/composables/useShareSheet.js)：`saveShare()` 仅 `toast = "图片已保存"`，无文件

### 12.5 轻量活动（V0.3，不做社区）

| 活动 | 形式 |
|------|------|
| 连续 3 天晒太阳 | 分享卡角标 |
| 带小芽看看春天 | 主题 invite 文案 |
| 校园探索 | 运营样片，非 UGC 系统 |

---

## 13. MVP 功能优先级

| 功能 | 产品价值 | 开发成本 | 实现程度（评审基线） | 优先级 | 决策 |
|------|---------|---------|---------------------|--------|------|
| 光照感知 | 高 | 中 | ✅ 固件 | **P0** | 保留 |
| IMU 移动 | 高 | 低 | ✅ | **P0** | 保留 |
| OLED + 彩屏 | 高 | 高 | ✅ | **P0** | 保留 |
| Wi-Fi + /plant | 高 | 中 | ✅ | **P0** | 保留 |
| 手机网页 | 高 | 中 | 15 屏 | **P0** | 精简 |
| Light Walk | 高 | 低 | 部分 | **P0** | 强化 |
| AI 日记 | 高 | 中 | 部分 | **P0** | 注入 sensor |
| 分享 PNG | 高 | 中 | ❌ | **P0** | 新增 |
| Color 拍照 | 中 | 中 | ✅ | **P1** | 保留，2 张 |
| TTS | 中 | 中 | ✅ | **P1** | 保留 |
| 历史/持久化 | 中 | 中 | ❌ | **P1** | JSON 文件 |
| growth_delta | 中 | 低 | ❌ | **P1** | +12/次 |
| Sound Walk | 中 | 中 | stub | **P2** | 隐藏 |
| ENV 首页 | 低 | 低 | ✅ | **P2** | 隐藏 |
| 图鉴 | 中 | 中 | 假解锁 | **P2** | 延后 |
| Nearby/Local/地图 | 低 | 低–高 | 静态 | **P3** | 删除路径 |
| 账号/GPS/AI 聊天 | 低 | 高 | ❌ | **P3** | 不做 |
| Matrix LED | 无 | — | 不存在 | **P3** | 删除宣传 |

---

## 14. 精确修改清单（问题 + 解决方案）

> 每条包含：问题 → 用户影响 → 修改对象 → 文件位置 → 修改前 → 修改后 → 优先级 → 工作量 → 负责人 → 验收标准

### 14.1 P0（必须做，不做无法形成 Demo 闭环）

#### P0-01 首页像仪表盘，无明确下一步

| 项 | 内容 |
|----|------|
| **问题** | 首页默认展示 4 行传感器（place/lux/声音/温湿度），无主 CTA |
| **用户影响** | 10 秒内不知道要做什么；以为智能硬件监控面板 |
| **修改对象** | 页面 |
| **修改位置** | `app_demo/08_vue_figma_strict_demo/src/views/HomeScreen.vue` |
| **修改前** | `LiveSensorList` + speech 卡 |
| **修改后** | 状态标题 + speech 卡 + **主按钮「出门晒太阳（约 3 分钟）」** 直接 `startWalk('light')`；传感器移至 `DemoNotesPanel` |
| **优先级** | P0 |
| **工作量** | S |
| **负责人** | 设计（布局）+ 你（逻辑） |
| **验收标准** | 5 名新用户 10 秒内能说出「带小芽出门晒太阳」 |

---

#### P0-02 底栏「散步」与 speech 入口不一致

| 项 | 内容 |
|----|------|
| **问题** | speech 卡用 `smartInvite`，底栏曾 `randomInvite` 随机 4 类型 |
| **用户影响** | 同一产品两种任务逻辑，信任感下降 |
| **修改对象** | 导航 |
| **修改位置** | `composables/useAppNavigation.js`、`components/FigmaBottomNav.vue`、各 Screen 的 `@smart-invite` |
| **修改前** | `randomInvite()` → 随机 type |
| **修改后** | `smartWalkInvite()` → `suggestInviteType(latest)`，默认 `light` |
| **优先级** | P0 |
| **工作量** | S |
| **负责人** | 你 |
| **验收标准** | 底栏与 speech 卡跳转同一 invite 类型 |

---

#### P0-03 分享卡是假的，无法发小红书

| 项 | 内容 |
|----|------|
| **问题** | ShareScreen 静态文案；保存仅 Toast |
| **用户影响** | 完成闭环无奖励；无法传播 |
| **修改对象** | 页面 + 依赖 |
| **修改位置** | `ShareScreen.vue`、`composables/useShareSheet.js`、`package.json`（html2canvas） |
| **修改前** | 写死 title/stats/essay |
| **修改后** | 绑定 `latest.walk_diary`（title、essay、walk_number、sunlight_seconds、growth_delta、photo）；`ref` 卡片 DOM → html2canvas → 下载 `city-sprout-share-{date}.png` |
| **优先级** | P0 |
| **工作量** | M |
| **负责人** | 设计（卡片 layout）+ 你（导出逻辑） |
| **验收标准** | 保存 PNG 含真实 essay 文本；文件可在相册打开 |

---

#### P0-04 Light Walk 无完成标准

| 项 | 内容 |
|----|------|
| **问题** | 用户不知何时停；室内可点「完成散步」 |
| **用户影响** | 任务无意义；AI 日记与真实外出脱节 |
| **修改对象** | 后端 + 前端 |
| **修改位置** | `backend/utils/storage.py`（`LIGHT_SUN_TARGET_SECONDS=180`）、`api/walk.py`（`/walk/progress`、`walk_completion_allowed`）、`WalkScreen.vue`（进度 UI）、`useWalkSession.js`（finish 传 sunlight_seconds） |
| **修改前** | 仅手动完成 |
| **修改后** | lux≥300 累计 180s（前端每 4s 轮询 `/latest` 计数 + POST `/walk/progress`）；`/walk/diary` 校验，未达标返回 400 `walk_not_complete`；达标后 Pause 页提示「晒够啦」 |
| **优先级** | P0 |
| **工作量** | M |
| **负责人** | 你 |
| **验收标准** | 室内纯静态无法生成 diary（除非 `SPROUT_DEMO_BYPASS_WALK=1`） |

---

#### P0-05 邀请文案含英文 Walk 类型

| 项 | 内容 |
|----|------|
| **问题** | `Light Walk`、`Color Walk` 等英文标签 |
| **用户影响** | 理解成本高；不像消费产品 |
| **修改对象** | 文案 |
| **修改位置** | `constants/demo.js` → `invites` |
| **修改前** | `walkTitle: "Light Walk"` |
| **修改后** | `walkTitle: "晒太阳散步"` 等全中文 |
| **优先级** | P0 |
| **工作量** | S |
| **负责人** | 设计 |
| **验收标准** | 首次路径无英文 Walk 类型 |

---

#### P0-06 AI 日记与真实散步脱节

| 项 | 内容 |
|----|------|
| **问题** | `diary.py` 仅读 session events，常为空则用假 timeline |
| **用户影响** | 两次散步 essay 高度相似；用户认为 AI 随机 |
| **修改对象** | AI Prompt |
| **修改位置** | `backend/ai/diary.py` |
| **修改前** | payload 无 lux/motion/place |
| **修改后** | `generate_walk_diary(session, latest_snapshot=...)` 注入传感器、时长、light_progress、photo summary；输出增加 `share_caption`、`discovered_elements` |
| **优先级** | P0 |
| **工作量** | M |
| **负责人** | 你 |
| **验收标准** | 不同 lux/有无关 photo 的两次 diary，essay 明显不同 |

---

#### P0-07 Sound Walk 假分析仍可达

| 项 | 内容 |
|----|------|
| **问题** | `ai/audio.py` 不分析文件；UI 展示「识别结果」 |
| **用户影响** | 信任崩塌 |
| **修改对象** | 产品路由 |
| **修改位置** | `router/index.js`（sound/local → redirect light）、`suggestInviteType`（仅 light/color） |
| **修改前** | 4 类型均可达 |
| **修改后** | Demo 主路径仅 light + color |
| **优先级** | P0 |
| **工作量** | S |
| **负责人** | 共同确认 |
| **验收标准** | 首次体验不出现 Sound/Local |

---

#### P0-08 Nearby 假社区占一级 Tab

| 项 | 内容 |
|----|------|
| **问题** | Nearby 6 张静态卡，无后端 |
| **用户影响** | 像画饼；分散注意力 |
| **修改对象** | 导航 |
| **修改位置** | `FigmaBottomNav.vue`（移除 nearby Tab）、`router/index.js`（`/nearby` → redirect `/home`） |
| **修改前** | 5 Tab 含附近 |
| **修改后** | 4 Tab：首页/散步/日记/我的 |
| **优先级** | P0 |
| **工作量** | S |
| **负责人** | 设计 + 你 |
| **验收标准** | 首次体验不见 Nearby |

---

#### P0-09 硬件 HTTP 失败导致全链断裂

| 项 | 内容 |
|----|------|
| **问题** | `SECRET_SERVER_URL` 用 `127.0.0.1` 或 IP 与 Flask 不一致 → `HTTP failed -1` |
| **用户影响** | OLED 英文兜底；首页无中文 speech |
| **修改对象** | 嵌入式 + 文档 |
| **修改位置** | `arduino_secrets.example.h`、`docs/VERIFICATION_CitySprout.md`、启动脚本打印 IP |
| **修改前** | 手动改 IP |
| **修改后** | 后端启动打印 `http://{lan_ip}:5000/plant`；验收清单勾选 |
| **优先级** | P0 |
| **工作量** | S |
| **负责人** | 你 |
| **验收标准** | 连续 3 次 Demo 无 HTTP -1 |

---

#### P0-10 walk 完成未与硬件传感器联动

| 项 | 内容 |
|----|------|
| **问题** | 固件不知 walk session；完成不校验 lux/motion |
| **用户影响** | 室内可「散步完成」 |
| **修改对象** | 后端 |
| **修改位置** | `api/plant.py` 调用 `tick_active_walk_from_plant`；`walk/diary` 校验 |
| **修改前** | 无校验 |
| **修改后** | 见 P0-04；调试设 `SPROUT_DEMO_BYPASS_WALK=1` |
| **优先级** | P0 |
| **工作量** | M |
| **负责人** | 你 |
| **验收标准** | 见 P0-04 |

---

### 14.2 P1（明显增强体验）

| # | 问题 | 修改位置 | 解决方案 | 负责人 | 验收 |
|---|------|---------|---------|--------|------|
| P1-11 | MP3 阻塞传感 | `.ino` loop L1928 | 去掉 `if (serviceMp3Playback()) return;`，MP3 与服务并行 | 你 | TTS 时 lux 仍更新 |
| P1-12 | OLED 更新慢 | `.ino` sensor 块 | `currentState != lastPlantState` → 立即 OLED | 你 | 状态变 1s 内可见 |
| P1-13 | 无 walk 历史 | `storage.py` | `generated/sprout_state.json` 持久化 diary_history、walk_count | 你 | 重启后见上次日记 |
| P1-14 | 无 growth_delta | `finish_diary` | 每次 +12，完成页展示 | 你 | 完成页有「成长 +12」 |
| P1-15 | 完成页无分享 CTA | `FinishScreen.vue` | 主按钮「生成分享卡」→ `/share` | 设计 | 1 击到达 Share |
| P1-16 | Color 5 张太重 | `COLOR_TARGET` | 改为 2 | 你 | 3 min 内可完成 Color |
| P1-17 | idle 未推 light | `suggestInviteType` | idle → light | 你 | idle 也进晒太阳邀请 |
| P1-18 | TTS 无 Key 英文 | `ai/tts.py` | 预置中文 MP3 库路径 | 你 | 无 Key 播中文 |
| P1-19 | 日记 loading 弱 | `PauseScreen.vue` | 文案「小芽正在写日记…」+ 禁用双点 | 设计 | 用户知在等待 |
| P1-20 | 图鉴默认全解锁 | `storage.py` | 初始仅 `Sunny Sprout` | 你 | 首次 1 项解锁 |

---

### 14.3 P2 / P3

| 优先级 | 项 | 方案 |
|--------|-----|------|
| P2 | ENV 进首页 | 仅 diary prompt 使用 |
| P2 | 地图页 | diary 完成后可选跳转，Demo 默认隐藏 |
| P2 | 麦克风 | place 辅助，失败 → unknown |
| P2 | share_caption 可编辑 | Share 页 textarea |
| P3 | 删除 | Nearby 路径、Matrix LED 宣传、Sound 真实分析（延后）、GPS、账号、社区 |

---

## 15. V0.1 / V0.2 / V0.3 版本规划

### 15.1 V0.1 — 稳定演示最小闭环

**目标**：用户 5–10 分钟内完成一次小芽出走，获得 AI 日记 + 分享 PNG。

| 类别 | 范围 |
|------|------|
| **必须** | POST /plant、首页精简、Light Walk、晒光完成规则、AI diary、分享 PNG |
| **模板替代** | TTS 可无 Key；Sound 关闭；地图/Nearby 隐藏 |
| **不做** | 账号、Sound 分析、GPS、社区 |
| **页面** | 首页、散步中、完成/日记、分享（4） |
| **接口** | `/plant` `/latest` `/walk/start` `/walk/progress` `/walk/diary` `/walk/photo`(可选) |
| **硬件** | DLight + IMU + OLED + Canvas |
| **验收** | `verify_city_sprout.ps1 -InProcess` + 人工 10 min + 1 张 PNG |

### 15.2 V0.2 — 次日再来

- walk 历史 3 条展示
- growth_delta、第 N 次出走文案
- 图鉴 2–3 项真实解锁
- Color Walk 可选 1 张照片增强分享图

### 15.3 V0.3 — 适合传播

- share_caption 可编辑
- 连续 3 天角标
- 主题任务「带小芽看看春天」
- 校园场景官方样片

---

## 16. 代码和 PRD 差异

| 类别 | PRD 描述 | 代码现状 | 文件 | 建议 |
|------|---------|---------|------|------|
| ✅ 已实现 | POST /plant + AI 文案 | 完整 + fallback | `api/plant.py`, `ai/speech.py` | 保留 |
| ✅ 已实现 | Color Walk 拍照 | VLM + 进度 | `api/walk.py`, `ai/vision.py` | COLOR_TARGET→2 |
| ⚠️ 低价值 | Nearby 推荐 | 静态假数据 | `NearbyScreen.vue` | 删除 Tab |
| ❌ 未实现 | 分享 PNG | Toast 假保存 | `ShareScreen.vue` | P0 实现 |
| ❌ 未实现 | 硬件参与 walk | 固件无 /walk | `.ino` | 通过 /plant 间接联动 |
| ⚠️ stub | Sound AI | 不读音频 | `ai/audio.py` | 隐藏 |
| ❌ 曾未实现 | 持久化 | 部分已起草 | `storage.py` | 完成并联调 |
| ❌ 未实现 | device_id 绑定 | 解析忽略 | `utils/state.py` | V0.2 |
| ❌ 冲突 | Matrix 5×5 LED | **不存在** | — | 文档修正 |
| ⚠️ 脱节 | 首页推荐任务 | 曾无 CTA | `HomeScreen.vue` | 主按钮 |
| ⚠️ 弱绑定 | 日记用传感器 | events 常空 | `diary.py` | 注入 latest |
| ⚠️ 阻塞 | Mic peak=0 | sound unknown | `.ino` init 顺序 | Mic 在 Canvas 前 init |
| ⚠️ 阻塞 | HTTP -1 | secrets IP | `arduino_secrets.h` | IP 清单 |
| ⚠️ OLED | 中文短句 | toOLEDAscii 剥离 | `.ino` | 保留常用汉字或拼音 |
| ⚠️ 潜在 bug | diary 签名 | walk.py 传 latest_snapshot | `diary.py` 未接参 | 对齐函数签名 |

### 16.1 硬件模块存在性（背景表 vs 仓库）

| 模块 | 背景表预期 | 仓库实际 |
|------|-----------|---------|
| AtomS3R | 主控 | ✅ |
| Atom-Matrix 5×5 LED | IMU+LED | ❌ 未接入 |
| DLight BH1750 | 光照 | ✅ PaHUB ch0 |
| ENV Pro | 温湿度/IAQ | ✅ PaHUB ch2 |
| PDM 麦克风 | 声音 | ✅ Voice Base |
| OLED 1.3" | 状态 | ✅ PaHUB ch1 |
| 手机 H5 | App | ✅ Vue 08 |
| Flask | 后端 | ✅ |
| 大模型 | 文案/日记 | ✅ qwen-plus |

---

## 17. 团队分工建议

| 事项 | 你 | 设计同学 | 共同确认 |
|------|-----|---------|---------|
| 状态机 / 完成规则 / diary prompt | ✅ | | ✅ 完成标准 |
| 固件 OLED / 阈值 / 联调 | ✅ | | |
| 首页 IA 减法、主按钮 | | ✅ | ✅ |
| 分享卡 layout + PNG 导出 UI | | ✅ | ✅ 字段 |
| 文案中文化 | | ✅ | ✅ |
| Demo 脚本 / 验收 | ✅ | | |
| 删除 Nearby / Sound 路径 | ✅ 路由 | ✅ 底栏 | ✅ |
| 用户测试 5–10 人 | ✅ 组织 | ✅ 记录 | ✅ |

**不优先**：CSS 颜色、字体、阴影、复杂动画——除非直接服务「看懂、玩得起来、愿意分享」。

---

## 18. Demo 验收方案

### 18.1 自动化指标

| 指标 | 方式 | 目标 |
|------|------|------|
| GET /latest 可达 | `verify_city_sprout.ps1 -InProcess` | PASS |
| POST /plant JSON + text/plain | 同上 | PASS |
| Color Walk 链路 | 同上 | PASS |
| walk/diary | 同上 | PASS |
| TTS ready | 有 Key 时 PASS；无 Key WARN | 可接受 |
| 分享 PNG | **人工** | 1 张真实 PNG |

### 18.2 体验指标

| 指标 | 目标 |
|------|------|
| 首次 10 分钟内完成率 | ≥80%（5–10 人） |
| 10 秒内理解产品 | ≥70% |
| 分享卡保存意愿 | ≥50% |
| 认为硬件有必要 | ≥60% |
| 认为 AI 日记有个性 | ≥50% |

### 18.3 最小用户测试

1. 不给说明，观察 30 秒能否说出产品是什么  
2. 引导完成 1 次 Light Walk  
3. 记录：卡点、是否愿保存分享图、是否愿第 2 天再来  
4. **通过线**：≥4/5 完成闭环且 ≥3/5 愿分享  

### 18.4 15 分钟演示脚本

1. 蔫小芽 OLED（2 min）  
2. 手机「出门晒太阳」（1 min）  
3. 户外 3 min，walking → sunlight（5 min）  
4. 日记 + **导出分享卡**（5 min）  
5. 口播：「小芽第 1 次出走日记」  

### 18.5 签字表

| 模块 | 自动化 | 人工 | 负责人 | 日期 |
|------|--------|------|--------|------|
| POST /plant + /latest | ☐ | ☐ | | |
| Light Walk 晒光完成 | ☐ | ☐ | | |
| AI diary | ☐ | ☐ | | |
| 分享 PNG | ☐ | ☐ | | |
| 硬件 Voice Base | ☐ | ☐ | | |
| Vue 首页 CTA | ☐ | ☐ | | |

---

## 19. 待确认问题

团队讨论时请优先拍板以下问题，再排期 P0：

| # | 问题 | 选项 | 影响 |
|---|------|------|------|
| 1 | Atom-Matrix 5×5 LED 是否仍规划？ | 接 / 不接 / 仅下一版 | 对外硬件描述 |
| 2 | Demo 是否允许「窗边晒光」作弊完成？ | 严格户外 / 允许窗边 / bypass 环境变量 | lux 阈值与验收 |
| 3 | TTS 是否 Demo 必过？ | 必须 / OLED+彩屏即可 | 联调优先级 |
| 4 | 目标演示日期与人天？ | 日期 + 人天 | P1 裁剪 |
| 5 | 分享卡必须含硬件实拍吗？ | 必须 / 插画即可 | Share 设计 |
| 6 | DASHSCOPE_API_KEY 演示是否稳定？ | 稳定 / 常失败 | AI vs 模板比例 |
| 7 | 首次默认任务？ | 仅 Light / Light+Color | 入口设计 |
| 8 | 室内调试模式？ | `SPROUT_DEMO_BYPASS_WALK=1` 是否正式提供 | 开发 vs 验收 |

---

## 20. 附录：仓库实现进度快照

> 本节记录评审后、文档编写时仓库的**部分实现**状态，便于讨论「哪些已做、哪些未做」。以 git 最新代码为准。

| 项 | 状态 | 说明 |
|----|------|------|
| 首页主按钮 | 🟡 部分 | `HomeScreen.vue` 已有 `home-walk-cta` |
| smartWalkInvite | 🟡 部分 | `useAppNavigation.js`、`FigmaBottomNav` 已改 |
| Nearby 隐藏 | 🟡 部分 | 底栏已移除；router redirect |
| 中文 invite 文案 | 🟡 部分 | `demo.js` 已改 |
| storage 持久化 + light_progress | 🟡 部分 | `storage.py` 已扩展 |
| walk 完成校验 + /walk/progress | 🟡 部分 | `walk.py` 已扩展 |
| COLOR_TARGET=2 | 🟡 部分 | storage 已改 |
| 图鉴初始仅 Sunny | 🟡 部分 | storage 已改 |
| diary 注入 latest | 🔴 未完成 | `walk.py` 调用带参，**diary.py 签名需对齐** |
| Share PNG 导出 | 🔴 未完成 | ShareScreen 仍为静态 |
| WalkScreen 晒光进度 UI | 🔴 未完成 | 前端未接 /walk/progress |
| FinishScreen 分享 CTA | 🔴 未完成 | |
| plant.py tick progress | 🔴 未完成 | |
| 固件 OLED 即时刷新 / MP3 不阻塞 | 🔴 未完成 | |
| VERIFICATION 文档更新 | 🔴 未完成 | |

---

## 核心结论（讨论用）

```text
产品方向对，Demo 形态不对。
第一版只讲一个故事：
  「小芽想晒太阳 → 带它出门 3 分钟 → 它写了一篇日记 → 发小红书」
技术已能跑，产品还没能卖。
最大缺口：分享 PNG、完成标准、首页 CTA——不是再多一个传感器。
```

---

**文档维护**：产品决策变更后更新第 19 节待确认项与第 20 节进度快照。实现完成后将对应条目标为 ✅ 并补充 PR 链接。
