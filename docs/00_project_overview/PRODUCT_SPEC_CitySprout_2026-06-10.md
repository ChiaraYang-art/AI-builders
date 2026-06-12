# 出走小芽 City Sprout 产品说明文档

**版本日期**：2026-06-10  
**项目阶段**：软硬件联动 Demo + Vue App 演示版  
**对应旧文档**：`PRD_CitySprout_软硬件一体版_2026-05-28.md`  
**当前重点**：稳定展示“硬件感知 → AI/后端处理 → 小芽表达 → App 互动与散步记录”的完整闭环。

---

## 1. 项目一句话

出走小芽是一棵可携带的 AI 小植物。它通过光照、移动、声音、温湿度等传感器感知自己所处的环境，并用温柔的植物口吻邀请用户带它出门看真实世界。

用户不是被提醒运动，而是在照顾一棵想见阳光、想听城市、想收集颜色的小芽。

---

## 2. 当前产品定位

当前项目不是一个完整上线 App，而是一个面向课程展示的软硬件一体 Demo。

Demo 需要让观众清楚看到三件事：

1. **小芽真的能感知环境**  
   硬件读取光照、移动、声音、温湿度等信息，并判断小芽状态。

2. **小芽会表达自己的感受**  
   后端根据传感器状态生成小芽文案，并可通过 TTS / Voice Base 形成语音表达。

3. **用户可以在 App 中和小芽一起完成散步任务**  
   Vue App 展示小芽状态、出门邀请、散步任务、照片上传、声音记录、完成反馈和小芽日记。

---

## 3. 当前系统组成

```text
硬件小芽
  ↓ 传感器数据 / 状态 / 文案请求
Flask 后端
  ↓ latest 状态 / walk 数据 / diary 数据
Vue App Demo
```

### 3.1 硬件小芽

硬件负责真实世界感知和线下表达。

当前硬件包含：

- M5Stack AtomS3R 主控
- AtomS3R 自带彩屏
- Atomic Voice Base 麦克风 / 扬声器
- PaHUB 分线器
- DLight 光照传感器
- Unit OLED 1.3 inch 黑白屏
- ENV-Pro 环境传感器
- AtomS3R 内置 IMU

当前硬件目标：

- 读取光照 lux
- 判断设备是否静止、移动、散步
- 检测声音环境变化
- 读取温湿度等环境数据
- 在 S3R 彩屏显示小芽动画
- 在 OLED 显示状态和短句
- 通过 Voice Base 播放小芽语音或预设语音

### 3.2 Flask 后端

后端负责接收硬件和 App 请求，并维护当前小芽状态。

当前后端能力：

- `POST /plant`：接收硬件上传的状态、光照、移动、声音、环境数据
- `GET /latest`：给 App 返回最新小芽状态、文案、传感器摘要、散步状态
- `POST /walk/start`：开始一次散步任务
- `POST /walk/photo`：Color Walk 上传照片
- `POST /walk/audio`：Sound Walk 上传环境音
- `POST /walk/diary`：生成散步日记
- `GET /walk/media/<walk_id>/<file>`：读取散步照片或音频文件
- `POST /settings/llm`：控制是否启用大模型

当前后端接入方向：

- 百炼 / Qwen 生成小芽文案
- DashScope TTS 生成小芽语音
- 图片识别用于 Color Walk
- 音频分析用于 Sound Walk
- 规则兜底用于节省 token 和避免演示失败

### 3.3 Vue App Demo

当前主力前端版本：

```text
app_demo/08_vue_figma_strict_demo/
```

App 目标：

- 尽量贴近 Figma 高保真视觉
- 保留可维护的 Vue 组件结构
- 与 Flask `/latest` 实时联动
- 支持出门邀请、四种散步任务、完成反馈、日记、附近、我的、图鉴等页面

右侧演示面板用于展示系统状态和快捷操作，当前文案为：

```text
出走小芽APP Demo
小芽状态与APP端实时联动。
```

---

## 4. 核心体验闭环

### 4.1 硬件感知闭环

```text
DLight / IMU / 麦克风 / ENV-Pro
  ↓
AtomS3R 判断小芽状态
  ↓
S3R 彩屏显示小芽动画
  ↓
OLED 显示短句
  ↓
POST /plant 上传后端
```

### 4.2 后端表达闭环

```text
POST /plant
  ↓
解析 state / lux / motion / sound / temp / humidity
  ↓
规则或大模型生成小芽文案
  ↓
可选生成 TTS
  ↓
更新 latest 状态
```

### 4.3 App 互动闭环

```text
App 首页轮询 GET /latest
  ↓
显示小芽状态和文案
  ↓
进入出门邀请
  ↓
开始 Light / Sound / Color / Local 任务
  ↓
上传照片或声音，记录散步状态
  ↓
完成散步并生成反馈页
  ↓
查看小芽日记 / 地图 / 图鉴
```

---

## 5. 小芽状态模型

当前代码主状态保持精简，避免状态爆炸。

| 状态 | 含义 | 典型触发 |
|---|---|---|
| `idle` | 安静等待 | 环境正常，暂时没有强提醒 |
| `wilted` | 有点蔫了 | 长时间低光照或缺少外界刺激 |
| `need_sun` | 想晒太阳 | 光照偏低，需要邀请用户出门 |
| `sunlight` | 得到阳光 | 光照充足 |
| `walking` | 正在散步 | IMU 检测到移动或散步状态 |

除主状态外，当前还有独立环境维度：

| 维度 | 字段示例 | 说明 |
|---|---|---|
| 光照 | `lux` | 判断缺光、室内、窗边、阳光 |
| 移动 | `motion` | `still` / `active` / `walking` |
| 声音 | `sound_state` | `unknown` / `quiet` / `lively` 等 |
| 位置推断 | `place` | `indoor` / `outside` / `unknown` |
| 环境 | `temperature_c` / `humidity_percent` | 用于文案和日记 |

---

## 6. App 当前页面结构

当前 Vue 路由包含：

| 页面 | 路由 | 功能 |
|---|---|---|
| 首页 | `/home` | 展示小芽状态、文案、传感器摘要 |
| 出门邀请 | `/invite/:type` | Light / Sound / Color / Local 邀请页 |
| 散步中 | `/walk/:type` | 对应任务进行页 |
| 暂停页 | `/pause/:type` | 继续散步或完成散步 |
| 完成反馈 | `/finish` | 普通散步完成反馈 |
| Color 完成反馈 | `/finish/color` | 与普通完成页类似，但下方展示用户照片 |
| 日记首页 | `/diary` | 日历 / 日记入口 |
| 日记流水账 | `/diary/log/:variant?` | 展示散步记录 |
| 小作文 | `/diary/essay/:variant?` | 展示小芽日记文本 |
| 地图 | `/map` | 展示散步地图 |
| 附近 | `/nearby` | 展示附近新店、新变化、树友发现 |
| 我的 | `/me` | 设备状态、语音音色、成长值、图鉴入口 |
| 图鉴 | `/atlas` | 展示已解锁的小芽形态 |
| 分享 | `/share` | 展示分享卡片 |

底部导航：

```text
首页 / 散步 / 日记 / 附近 / 我的
```

当前特殊交互：

- 首页点击小芽说话框，进入四种出门邀请之一。
- 底部导航“散步”也进入四种出门邀请之一。
- Color Walk 完成页会在下方展示用户上传的照片。
- 其它任务完成页统一使用普通散步完成反馈页。

---

## 7. 四种散步任务

### 7.1 Light Walk

目标：带小芽去接触真实阳光。

当前体验：

- 展示光照、移动、温湿度等摘要
- 根据硬件上报状态展示小芽反馈
- 如果未达到足够光照就点“完成散步”，会弹出提示：

```text
好像还没接收到足够的光呢，现在就要回去嘛？
```

用户可以选择：

```text
仍然结束
```

点击后进入普通散步完成反馈页。

### 7.2 Sound Walk

目标：让小芽听见城市里的真实声音。

当前体验：

- App 页面展示声音卡片和小芽评论
- 支持浏览器录一段环境音并上传后端
- 线下硬件侧可通过麦克风检测声音环境变化
- 当前网页端不播放小芽语音，语音表达交给硬件小芽

说明：Sound Walk 当前偏演示版，重点展示“声音环境被记录和反馈”，不是完整的语音识别产品。

### 7.3 Color Walk

目标：带小芽收集城市中的绿色。

当前体验：

- 用户在 App 上传一张绿色照片
- 上传前，小芽提示：

```text
小芽期待看到城市里的颜色
```

- 上传后，小芽提示：

```text
我看到你找的绿色了！这片绿色看起来好有生命力呀
```

- 如果未上传照片就点“完成散步”，会弹出提示：

```text
还没收集到颜色呢，今天要先跳过嘛？
```

用户可以选择：

```text
确定跳过
```

- Color Walk 完成反馈页顶部仍显示小芽，照片放在下方，用户可以向下滑动查看。

### 7.4 Local Discovery

目标：引导用户关注附近新变化。

当前体验：

- 作为四种出门邀请之一
- 不等同于底部导航“附近”页面
- 底部导航“附近”展示附近新店、新变化、树友发现等信息流

当前 Local Discovery 暂按预设内容演示，不接真实小红书 / 大众点评 API。

---

## 8. AI 与 Token 策略

当前产品不要求每一次状态变化都调用大模型。

推荐策略：

1. **高频状态变化走规则文案**  
   例如 idle、need_sun、sunlight、walking 的基础短句可本地或后端规则生成。

2. **关键节点调用大模型**  
   例如：
   - 小芽正式邀请用户出门
   - 用户上传 Color Walk 照片后
   - 用户上传 Sound Walk 音频后
   - 散步结束生成日记

3. **TTS 优先服务线下硬件表达**  
   网页端不再播放小芽语音，避免和硬件声音重复。

4. **保留规则兜底**  
   当 API Key 未配置、网络失败、token 不足时，后端仍返回稳定演示文案。

---

## 9. 当前主程序和文件说明

### 9.1 Arduino

推荐关注以下目录：

| 目录 | 用途 |
|---|---|
| `arduino/city_sprout_pahub_main_v5_roadshow_demo/` | 传感器场景路演版，展示 6 段状态循环 |
| `arduino/city_sprout_pahub_main_v6_growth_demo/` | 成长循环 Demo，展示小芽逐渐长大 |
| `arduino/city_sprout_pahub_main_v4_no_flicker_canvas/` | PaHUB 主程序集成版，减少屏幕闪烁 |
| `arduino/archive/dlight_imu_sound_flask/` | 光照 + 移动 + 声音 + Flask 测试版 |
| `arduino/hardware_tests/voice_base_mic_test/` | Voice Base 麦克风测试 |
| `arduino/hardware_tests/speaker_hardware_test/` | 扬声器测试 |
| `arduino/archive/dual_screen_state_loop/` | 双屏状态轮播视觉测试 |

### 9.2 后端

主目录：

```text
backend/
```

推荐启动：

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders"
python backend\sprout_server.py
```

运行后应看到：

```text
Running on http://127.0.0.1:5000
Running on http://<本机局域网 IP>:5000
```

### 9.3 前端 App

主目录：

```text
app_demo/08_vue_figma_strict_demo/
```

推荐启动：

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders\app_demo\08_vue_figma_strict_demo"
npm run dev
```

浏览器打开：

```text
http://localhost:5173/
```

如果终端显示端口变成 `5174` 或其它端口，则打开终端显示的地址。

---

## 10. 当前演示启动流程

关机重启后，需要分别启动后端和前端。

### 10.1 启动后端

PowerShell 窗口 1：

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders"
python backend\sprout_server.py
```

### 10.2 启动前端

PowerShell 窗口 2：

```powershell
cd "C:\Users\yqe\Desktop\CitySproutDemo\AI builders\app_demo\08_vue_figma_strict_demo"
npm run dev
```

### 10.3 硬件端注意

Arduino 中服务器地址不能写 `127.0.0.1`。

硬件需要访问电脑或云服务器的地址，例如：

```text
http://<电脑局域网 IP>:5000/plant
```

或云服务器：

```text
http://<云服务器 IP>:5000/plant
```

真实 Wi-Fi、服务器地址、API Key 不应提交到 GitHub。

---

## 11. 当前已经完成的关键进展

### 11.1 硬件侧

- AtomS3R 可运行 Arduino 程序
- S3R 彩屏可显示小芽动画
- OLED 可横屏显示文本
- DLight 光照传感器已跑通
- IMU 移动检测已整合进测试链路
- Voice Base 麦克风可测试
- Voice Base 扬声器可播放
- PaHUB 到货后已形成多模块连接方案
- v5 路演版和 v6 成长版已形成独立目录

### 11.2 后端侧

- Flask 服务已可运行
- `/plant` 与硬件联动
- `/latest` 与 App 联动
- `/walk/start`、`/walk/photo`、`/walk/audio`、`/walk/diary` 已具备
- 支持规则文案兜底
- 预留百炼 Qwen / TTS 接入
- 支持散步媒体文件保存

### 11.3 App 侧

- 已从 Figma 高保真设计转为 Vue 版 Demo
- 已使用导出的 SVG/图片资产替换复杂插画
- 已有 Home、Invite、Walk、Pause、Finish、Diary、Nearby、Me、Atlas、Share 等页面
- Color Walk 支持上传照片
- Sound Walk 支持录音上传
- 完成散步支持未完成弹窗和跳过
- Color Walk 完成页与普通完成页分离
- 网页端已移除小芽语音播放按钮，语音交给线下硬件

---

## 12. 当前限制与已知问题

### 12.1 数据持久化

当前散步 session 主要保存在后端运行时状态中。

照片文件会保存到后端目录：

```text
backend/generated/walks/
```

但如果后端重启，当前 active walk 可能丢失。因此当前仍是演示级数据管理，不是正式用户相册。

### 12.2 AI 调用成本

如果每次状态变化都调用大模型，token 消耗会很快。

当前建议：

- 高频状态使用规则文案
- 散步结束、照片总结、声音总结等关键节点再调用大模型
- 演示时可关闭 LLM 或使用规则兜底

### 12.3 App 仍是 Demo

当前 Vue App 重点是展示交互逻辑和视觉效果。

尚未包含：

- 用户账号
- 真正的手机端打包
- 真实 GPS 轨迹
- 真实地图服务
- 正式数据库
- 正式对象存储
- 完整权限管理

### 12.4 Local Discovery 暂未接真实平台

当前“附近”和 Local Discovery 使用预设内容和本地资产。

后续如果要商业化，可考虑接：

- 小红书内容
- 大众点评 / 地图 POI
- 用户上传的附近发现

---

## 13. 后续建议路线

### P0：决赛 / 课程展示稳定性

- 保证硬件能稳定上报 `/plant`
- 保证 App 首页实时更新
- 保证四种邀请页能进入
- 保证 Color Walk 上传照片并显示反馈
- 保证未完成任务也能跳过到完成页
- 保证 Voice Base 有可听见的语音输出

### P1：体验完整度

- 让散步日记内容更贴合当前任务类型
- 优化 Sound Walk 录音后的反馈文案
- 优化 Color Walk 图片识别与颜色总结
- 增加成长值与图鉴解锁逻辑
- 增加更稳定的数据持久化

### P2：真实产品方向

- 手机 App 打包
- 用户账号和云端同步
- 数据库和对象存储
- 真实 GPS / 地图
- 真实附近内容推荐
- 大模型多模态日记生成
- 更完整的硬件语音对话

---

## 14. 当前 Demo 推荐讲述方式

推荐现场叙事：

1. 小芽长期待在室内，有点蔫。
2. 它通过光照、声音和移动感知世界。
3. 它不是提醒用户运动，而是请求用户照顾它。
4. 用户在 App 里接受一种散步邀请。
5. 出门后，小芽看到阳光、听到城市、收集颜色。
6. 散步结束后，小芽把这次经历写成日记。
7. 小芽会慢慢成长，并解锁不同形态。

这一版最重要的表达不是“健康打卡”，而是：

```text
用户带一棵想见真实世界的小植物出门。
```

