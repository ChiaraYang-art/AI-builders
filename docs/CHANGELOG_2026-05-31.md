# City Sprout 改进总结（2026-05-31）

本文档汇总 2026-05-31 会话中对「出走小芽 / City Sprout」项目的主要改进，便于提交代码与路演前自检。

---

## 一、改进概览

| 类别 | 主要成果 |
|------|----------|
| 硬件固件 | 预录 RAM 播放、语音互斥、20s 统一间隔、传感器阈值与演示防抖、网页 LLM 开关同步 |
| 后端 | TTS 音色统一、LLM 开关 API、关闭 LLM 时走规则文案 |
| 前端 | 首页重叠修复、「我的」页大模型开关 |
| 语音 | 实时 TTS 与预录库统一为 `longhuhu_v3` 女童声 |

---

## 二、硬件固件（Arduino）

**主程序：** `arduino/city_sprout_pahub_main_v4_no_flicker_canvas/city_sprout_pahub_main_v4_no_flicker_canvas.ino`

### 2.1 预录音频卡顿修复

**问题：** 预录 MP3 文件本身正常（约 45–131 KB，24 kHz），卡顿来自 ESP32 **WiFi 边下边播**（HTTP Stream），而非文件损坏。

**方案：**
- 新增 `AudioFileSourceMemory`、`downloadMp3ToRam()`、`startLibraryMp3Playback()`
- 待机预录：**整段下载到 RAM 后再播放**
- 扬声器采样率设为 **24000 Hz**，与 CosyVoice `MP3_24000HZ` 一致
- 内存播放时 `serviceMp3Playback()` 不再 pump 网络缓冲

**串口验证：**
```
Downloading library MP3: ...
MP3 downloaded to RAM: 53371 bytes
Library MP3 playback started from RAM.
```

> 实时演示 TTS（`latest.mp3`）仍使用流式播放；文件较大（约 270 KB+），演示语音仍可能略卡，与预录路径分离。

### 2.2 演示语音 vs 待机预录（互斥）

**期望逻辑：**
- **演示**（传感器变化）：`POST /plant` → LLM → 只播 TTS（`latest.mp3`）
- **待机**（静置）：不调 LLM → 每 20 秒播一条预录
- 两种声音互斥，不可叠加

**实现要点：**
- `demoAudioCycleActive`：TTS 等待/播放期间禁止预录
- `markAudioPlaybackFinished()`：播完重置计时
- `canPlayIdleLibraryAudio()` / `runIdleLibraryAudio()`：严格间隔与互斥判断
- 移除演示 POST 后错误重置待机计时导致的「刚播完又立刻预录」问题

### 2.3 语音间隔统一为 20 秒

**问题：** 演示模式原最短间隔 **8 秒**，传感器抖动时设备「一直在说」，20 秒待机间隔形同虚设。

**方案：**
- 统一常量 `SPEECH_INTERVAL_MS = 20000`
- 演示与待机均从 **上一段语音播完** 起算 20 秒
- 演示仍优先于待机；传感器须 **稳定 4 秒** 才触发 LLM

### 2.4 传感器阈值与演示防抖

**问题：** 原阈值偏低，轻微桌面震动、环境声、光照边界抖动即可触发 `hasDemoTrigger()` → 反复调大模型。

**调整：**

| 参数 | 原值 | 新值 | 说明 |
|------|------|------|------|
| `ACTIVE_THRESHOLD` | 0.06 | **0.10** | 运动判定更不敏感 |
| `ACTIVE_HOLD_TIME_MS` | 1500 | **2200** | 减少 still/active 抖动 |
| `WALK_ENTER_LEVEL` | 0.060 | **0.095** | 散步态需更明显晃动 |
| `LUX_WILTED_MAX` | 50 | **30** | 需更暗才算蔫 |
| `LUX_NEED_SUN_MAX` | 300 | **450** | 室内亮灯不易跳 sunlight |
| `DEMO_TRIGGER_STABLE_MS` | 2.5s | **4s** | 变化须稳定才演示 |
| 声音 quiet/active 阈值 | 较低 | **略提高** | 减少 place 闪烁 |

**逻辑优化：**
- 已在 `STATE_WALKING` 时，忽略 motion still/active 抖动触发的 LLM
- `place` 在 `indoor ↔ unknown` 间闪烁时不再触发演示
- 演示触发增加 `isDemoTriggerReady()` 防抖

### 2.5 网页端 LLM 开关同步（硬件侧）

- 新增 `refreshLlmDemoSetting()`：约每 **10 秒** 读 `GET /latest` 中的 `llm_enabled`
- 关闭时：`runDemoModePost()` 跳过 `POST /plant`，串口输出 `LLM demo disabled via web`
- 关闭时：取消进行中的 TTS 等待

**需重新烧录固件后生效。**

### 2.6 其他固件修复

- 补充 `showTextOnOLED` 等前向声明，修复编译错误 `was not declared in this scope`

---

## 三、后端（Flask）

### 3.1 TTS 音色统一

- 实时 TTS 由 `longanyang` 男声改为 **`longhuhu_v3`** 女童声
- 与预录库 `VoiceGenerate/tts_output/` 音色一致
- 修改：`backend/.env`、`backend/config.py`、`backend/ai/tts.py`、`backend/api/latest.py`（返回 `tts_voice` / `tts_model`）
- 启动日志应含：`TTS configured: model=cosyvoice-v3-flash, voice=longhuhu_v3`

### 3.2 大模型开关 API

**新增：** `backend/api/settings.py`

| 接口 | 说明 |
|------|------|
| `GET /settings/llm` | 返回 `{ "llm_enabled": true/false }` |
| `POST /settings/llm` | Body: `{ "llm_enabled": true/false }` |

**存储：** `backend/generated/sprout_state.json` 持久化 `llm_enabled`（默认 `true`）

**`GET /latest`：** 增加字段 `llm_enabled`

**`POST /plant`：**
- `llm_enabled=true`：调用 LLM + 启动 TTS 合成
- `llm_enabled=false`：仅用 `generate_rule_speech()` 规则文案，**不调用大模型、不合成 TTS**

### 3.3 部署注意

修改后端后需 **重启 Flask**（`python sprout_server.py`），否则仍跑旧代码。

---

## 四、前端（Vue 08 Demo）

**路径：** `app_demo/08_vue_figma_strict_demo/`

### 4.1 首页文字重叠修复

- **原因：** `HomeScreen.vue` 同时渲染 `LiveSensorList` 与 `.home-speech`，CSS 均为 `top: 136px` 重叠
- **修复：** 从首页移除 `LiveSensorList`；传感器信息仍在右侧 `DemoNotesPanel` 显示

### 4.2 大模型对话开关

- **位置：** 底部导航 **「我的」** → 设置第一项 **「大模型对话」**
- **说明：** 与右侧调试面板的 **「开启 TTS / 开启语音」** 不同——后者只控制网页是否播放已合成音频，不控制硬件是否调 LLM
- **实现：** `MeScreen.vue` + `api/sprout.js` 的 `updateLlmEnabled()`

### 4.3 开发访问

```bash
cd app_demo/08_vue_figma_strict_demo
npm run dev
```

默认 `http://localhost:5173` 或 `5174`（端口占用时自动切换）。API 通过 Vite 代理到 `http://127.0.0.1:5000`。

---

## 五、路演操作建议

### 只播预录、不调大模型

1. 网页 **「我的」→ 关闭「大模型对话」**
2. 设备平放、少晃动
3. 约 20 秒一条待机预录；串口见 `Idle mode: library clip started`

### 演示 AI 对话

1. **打开「大模型对话」**（硬件约 10 秒内同步）
2. 等上一条播完 **20 秒**
3. 遮光或摇晃，保持变化 **约 4 秒**
4. 串口见 `Demo mode: sensor changed, calling LLM`

### 语音互斥

- 演示 TTS 播放期间不会插播预录
- 预录播放期间不会进入 LLM 等待

---

## 六、主要改动文件清单

```
arduino/city_sprout_pahub_main_v4_no_flicker_canvas/city_sprout_pahub_main_v4_no_flicker_canvas.ino
backend/.env
backend/config.py
backend/ai/tts.py
backend/api/plant.py
backend/api/latest.py
backend/api/settings.py          # 新增
backend/api/__init__.py
backend/utils/storage.py
app_demo/08_vue_figma_strict_demo/src/views/HomeScreen.vue
app_demo/08_vue_figma_strict_demo/src/views/MeScreen.vue
app_demo/08_vue_figma_strict_demo/src/api/sprout.js
app_demo/08_vue_figma_strict_demo/src/styles.css
deploy/.env                      # TTS 音色（如有）
```

---

## 七、提交前自检清单

- [ ] 已烧录最新 `.ino` 到 AtomS3
- [ ] Flask 已重启，`GET /latest` 含 `llm_enabled`、`tts_voice=longhuhu_v3`
- [ ] Vue dev 可访问，「我的」页开关可切换
- [ ] 关闭 LLM 后设备串口出现 `LLM demo disabled via web`
- [ ] 预录播放串口出现 `MP3 downloaded to RAM`
- [ ] 静置约 30s 仅一条预录，间隔约 20s

---

## 八、已知限制 / 后续可选

1. **实时 TTS 仍流式播放**，大段 `latest.mp3` 在硬件上可能略卡；可选改为与预录相同的 RAM 整段下载
2. **Vue 日记页** 部分仍为 demo 静态数据，未完全接入 `diary_history` API
3. 传感器阈值可按实际路演环境继续微调（当前偏「减少误触发」）

---

*文档生成日期：2026-05-31*
