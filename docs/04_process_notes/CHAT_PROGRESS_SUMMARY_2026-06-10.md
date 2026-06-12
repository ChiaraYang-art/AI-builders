# 出走小芽本对话进度总结

**日期**：2026-06-10  
**用途**：用于多个聊天框内容合并后的项目交接。  
**范围**：本文件只总结当前这个对话框中完成的推进，不替代完整 PRD 或代码 README。

---

## 1. 本对话总体进展

在这个对话中，项目从早期“可携带 AI 小芽 / 出门提醒器”的硬件 Demo，推进到一个较完整的软硬件联动展示系统。

当前系统包含三条主线：

```text
AtomS3R 硬件小芽
  ↓
Flask 后端
  ↓
Vue App Demo
```

本对话主要完成了：

- 梳理硬件主程序和传感器链路
- 整理 Flask 后端和百炼 / Qwen / TTS 接入方向
- 从 Figma 设计稿逐步开发 Vue App Demo
- 修复多个 App 页面和交互问题
- 整理启动方式、项目文档和当前产品说明

---

## 2. 重要产品方向确认

### 2.1 核心母题

项目从“AI 和低侵入硬件帮助人重新感知城市中的自然生命”，收敛为：

```text
一个可携带的 AI 小植物小芽，通过光照、声音、移动和环境数据，
邀请用户带它出门看真实世界。
```

关键表达不是健康打卡，而是：

```text
用户不是被提醒运动，而是在照顾一棵想见真实世界的小植物。
```

### 2.2 当前 Demo 目标

当前 Demo 的重点是：

```text
传感器数据
  ↓
小芽状态
  ↓
后端文案 / 语音
  ↓
硬件显示 / 播放
  ↓
App 同步
  ↓
散步任务和日记
```

### 2.3 网页端声音策略

已确认：

- 网页端不再播放小芽语音
- 小芽语音交给线下硬件 Voice Base
- App 只显示状态、文案和任务信息

---

## 3. 硬件侧进展

### 3.1 硬件清单和连接方向

当前硬件方案：

```text
AtomS3R + Atomic Voice Base
  ↓
PaHUB
  ├── 0 号口：DLight 光照传感器
  ├── 1 号口：OLED
  └── 2 号口：ENV-Pro
```

同时使用：

- AtomS3R 自带彩屏
- AtomS3R 内置 IMU
- Atomic Voice Base 麦克风 / 扬声器

### 3.2 已推进的硬件功能

本对话中围绕硬件完成或整理过：

- DLight 光照读取
- IMU 移动 / 行走检测
- 声音环境变化检测
- Atomic Voice Base 麦克风测试
- Atomic Voice Base 扬声器测试
- PaHUB 多传感器整合
- OLED 文本显示
- S3R 彩屏小芽动画
- TTS / 预设音频播放方向

### 3.3 当前主要 Arduino 目录

推荐后续重点关注：

```text
arduino/city_sprout_pahub_main_v5_roadshow_demo/
arduino/city_sprout_pahub_main_v6_growth_demo/
arduino/city_sprout_pahub_main_v4_no_flicker_canvas/
arduino/dlight_imu_sound_flask/
arduino/voice_base_mic_test/
arduino/speaker_hardware_test/
```

### 3.4 v5 / v6 硬件 Demo 区分

v5：

```text
city_sprout_pahub_main_v5_roadshow_demo
```

用途：

- 传感器场景路演
- 展示 idle / dark / need sun / walk / sunlight / city sound 等状态循环

v6：

```text
city_sprout_pahub_main_v6_growth_demo
```

用途：

- 展示小芽成长循环
- 适合讲“小芽慢慢长大”的故事线

---

## 4. Flask 后端进展

### 4.1 后端核心能力

后端已整理为 Flask 服务，主入口：

```text
backend/sprout_server.py
```

当前涉及的主要接口：

| 接口 | 用途 |
|---|---|
| `POST /plant` | 硬件上传小芽状态和传感器数据 |
| `GET /latest` | App 获取最新状态、文案和散步信息 |
| `POST /walk/start` | 开始散步任务 |
| `POST /walk/photo` | Color Walk 上传照片 |
| `POST /walk/audio` | Sound Walk 上传录音 |
| `POST /walk/diary` | 生成散步日记 |
| `GET /walk/media/<walk_id>/<file>` | 读取散步照片或声音文件 |
| `POST /settings/llm` | 控制是否启用大模型 |

### 4.2 AI / 百炼 / Qwen 方向

本对话中讨论并整理过：

- 百炼 Qwen 作为小芽文案和语音生成后端
- API Key 不应写入 GitHub
- 本地后端和云服务器两种运行方式
- 由于 token 消耗较快，建议不要所有状态都调用大模型

当前推荐策略：

```text
高频状态变化：规则文案
关键节点：调用大模型
散步结束：生成日记
语音播放：优先硬件端
```

### 4.3 force_complete 改动

为了解决 Light Walk / Color Walk 未完成时无法结束散步的问题，后端增加了单次强制完成参数：

```json
{
  "walk_id": "...",
  "force_complete": true
}
```

对应接口：

```text
POST /walk/diary
```

注意：如果运行的是旧版云端后端，可能还没有这个参数；前端已经加了兜底跳转。

---

## 5. Vue App Demo 进展

### 5.1 当前主力版本

当前主力 App Demo：

```text
app_demo/08_vue_figma_strict_demo/
```

这是基于 Figma 高保真稿开发的 Vue + Vite 版本。

运行方式：

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders\app_demo\08_vue_figma_strict_demo"
npm run dev
```

浏览器打开：

```text
http://localhost:5173/
```

### 5.2 当前页面

当前 Vue 路由包含：

```text
/home
/invite/:type
/walk/:type
/pause/:type
/finish
/finish/color
/diary
/diary/log/:variant?
/diary/essay/:variant?
/map
/nearby
/me
/atlas
/share
```

底部导航：

```text
首页 / 散步 / 日记 / 附近 / 我的
```

### 5.3 Figma 设计转 Vue 的过程

本对话中从 Figma 设计稿推进过：

- 初版 HTML 原型
- 图片热点版
- 组件版
- Vue 版
- Figma 视觉优先版
- 使用导出的 SVG/图片资产提升还原度

当前结论：

- 复杂插画、地图、小芽形象优先用 Figma 导出的 SVG/PNG
- 文字、按钮、路由、状态、上传逻辑用 Vue + CSS
- 这样既能贴近视觉，也便于程序同学后续加功能

### 5.4 已替换和使用的主要资产

资产来源：

```text
app_demo/figma_assets_raw/
```

App 使用位置：

```text
app_demo/08_vue_figma_strict_demo/public/assets/figma/
```

本对话中更新过：

- `sprout_idle 1.svg`
- `sprout_need_sun 3.svg`
- `sprout_sunlight 2.svg`
- `sprout-wilted 2.svg`
- `diary-page-sprout.svg`
- `share-card-sprout.svg`
- `sprout-comment.svg`
- `sprout-color walk comment.svg`
- `sprout-local discovery comment.svg`
- `nearby pic 1.svg` 等附近页图片

---

## 6. App 关键交互逻辑

### 6.1 首页到邀请页

保留的产品逻辑：

- 首页点击小芽说话框，进入四种出门邀请之一
- 底部导航“散步”也进入四种出门邀请之一

四种邀请：

```text
Light Walk
Sound Walk
Color Walk
Local Discovery
```

### 6.2 暂停和完成散步

散步页左上角返回进入暂停页：

```text
/pause/:type
```

暂停页可以：

- 继续散步
- 完成散步

Light Walk 未完成时弹窗：

```text
好像还没接收到足够的光呢，现在就要回去嘛？
```

按钮：

```text
仍然结束
```

Color Walk 未完成时弹窗：

```text
还没收集到颜色呢，今天要先跳过嘛？
```

按钮：

```text
确定跳过
```

点击后进入散步完成反馈页。

### 6.3 完成页区分

当前已修复：

- `/finish`：普通散步完成反馈页
- `/finish/color`：Color Walk 专属完成页

Color Walk 完成页：

- 顶部与普通完成页基本一致
- 不再把用户照片放在顶部 hero 区
- 用户上传的照片显示在下方
- 页面可以向下滑动

其它任务完成页：

- 与 Light Walk 普通完成页一致
- 不会因为上一次 Color Walk 日记而误显示“今天我收集了很多绿色”

### 6.4 Color Walk 上传逻辑

Color Walk 上传前显示：

```text
小芽期待看到城市里的颜色
```

上传至少一张照片后显示：

```text
我看到你找的绿色了！这片绿色看起来好有生命力呀
```

当前照片逻辑：

- 前端通过 `POST /walk/photo` 上传照片
- 后端保存到 `backend/generated/walks/`
- 只要后端当前 session 还在，刷新页面后仍能显示
- 如果后端重启，当前 active walk 可能丢失，但图片文件本身还在目录里

### 6.5 Sound Walk 逻辑

当前 Sound Walk：

- 页面展示声音卡片
- 支持浏览器录音上传
- 小芽评论条已修复文字错位
- 网页端不播放小芽声音

---

## 7. 最近一次前端修改清单

本对话后半段集中修了 App 08：

### 7.1 移除网页声音播放

已去掉：

- 首页声音按钮
- Demo 面板 TTS 按钮
- 浏览器自动播放小芽语音逻辑

保留：

- Sound Walk 环境音录制上传

### 7.2 Demo 面板修改

右侧 Demo 面板现在显示：

```text
City Sprout
出走小芽APP Demo
小芽状态与APP端实时联动。
```

按钮保留：

```text
首页 / 立即刷新 / 随机散步
```

按钮下方增加提示：

```text
如果无法点击“完成散步”，请多点击几次
```

### 7.3 修复过的页面问题

已修：

- Color Walk 评论文案重叠
- Color Walk 上传后文案切换
- Sound Walk 评论条文字错位
- 完成页小芽和标题重叠
- Color Walk 完成页照片位置
- Light Walk 强行结束误跳 Color Walk 日记
- 右侧面板标题和快捷按钮
- “我的”页面部分文字过大问题
- 日记小作文页使用新素材
- 附近页图片替换为 `nearby pic` 系列资产

---

## 8. 文档产出

本对话中生成或更新的重要文档：

### 8.1 新版产品说明文档

```text
docs/PRODUCT_SPEC_CitySprout_2026-06-10.md
```

用途：

- 替代旧 PRD 作为当前阶段说明
- 说明当前系统组成、页面、散步任务、AI 策略、运行方式、限制和后续路线

### 8.2 本对话进度总结

当前文件：

```text
docs/CHAT_PROGRESS_SUMMARY_2026-06-10.md
```

用途：

- 汇总当前聊天框中推进过的事项
- 供后续与其它聊天框总结合并
- 作为新对话继续推进时的上下文

---

## 9. 当前运行方式

关机重启后，需要分别启动后端和前端。

### 9.1 启动后端

PowerShell 窗口 1：

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders"
python backend\sprout_server.py
```

看到类似：

```text
Running on http://127.0.0.1:5000
```

表示后端启动成功。

### 9.2 启动前端

PowerShell 窗口 2：

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders\app_demo\08_vue_figma_strict_demo"
npm run dev
```

浏览器打开终端显示的地址，通常是：

```text
http://localhost:5173/
```

如果显示 `5174` 或其它端口，就打开终端显示的实际地址。

### 9.3 硬件服务器地址注意

Arduino 不能使用：

```text
127.0.0.1
```

硬件必须访问电脑局域网 IP 或云服务器地址，例如：

```text
http://<电脑局域网 IP>:5000/plant
```

---

## 10. 当前限制和风险

### 10.1 后端数据不是正式持久化

照片文件会保存，但 active walk session 主要在后端运行时内存中。

如果 Flask 重启：

- 当前散步可能丢失
- 页面可能无法恢复刚才的 active walk

### 10.2 云端后端可能不是最新代码

如果使用云服务器后端，而云端没有同步最新代码，可能出现：

- `force_complete` 不生效
- 完成散步仍返回 `walk_not_complete`
- App 只能走前端兜底跳转

### 10.3 多处旧文件存在编码乱码

部分旧文档和旧 Vue 文件显示为乱码，疑似编码历史问题。

后续建议：

- 新文档统一使用 UTF-8
- 不再直接编辑乱码旧文档
- 重要说明另存为新文件

### 10.4 App 仍是演示版

当前 App 没有：

- 正式登录
- 真实数据库
- 手机端打包
- 真实 GPS
- 真实地图服务
- 正式云存储

---

## 11. 建议下一步

### 11.1 多聊天框整合后

建议新对话先读取：

```text
docs/PRODUCT_SPEC_CitySprout_2026-06-10.md
docs/CHAT_PROGRESS_SUMMARY_2026-06-10.md
```

再读取其它聊天框总结文档。

### 11.2 决赛 / 展示前优先检查

优先级最高：

- 后端能启动
- Vue 能启动
- 首页能显示小芽状态
- Light Walk 能进入、暂停、完成
- Color Walk 能上传照片、完成页显示照片
- Sound Walk 页面不再文字错位
- 硬件能上报 `/plant`
- Voice Base 能发声

### 11.3 程序同学后续建议

如果继续工程化：

- 把后端 session 持久化到 JSON 或 SQLite
- 把上传照片保存路径和 URL 做得更稳定
- 把乱码文件逐步 UTF-8 化
- 把 Vue 中硬编码坐标逐步抽成组件样式
- 给完成页、Color Walk、Sound Walk 加最小回归测试

---

## 12. 本对话最终状态一句话

本对话把“出走小芽”推进为一个可演示的软硬件联动系统：硬件能感知，后端能汇总和生成表达，Vue App 能展示小芽状态、四种散步任务、照片/声音记录、完成反馈和小芽日记；同时修复了大量展示前会影响观感和流程的交互细节。

