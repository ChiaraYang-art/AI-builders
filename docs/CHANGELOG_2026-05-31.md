# City Sprout 改进总结（2026-05-31）

本文档汇总 2026-05-31 会话中对「出走小芽 / City Sprout」项目的主要改进，便于提交代码与路演前自检。

---

## 一、改进概览

| 类别 | 主要成果 |
|------|----------|
| 硬件固件 | 预录 RAM 播放、30s 定时上报 LLM、20s 语音间隔、网页 LLM 开关同步 |
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

### 2.2 语音调度：30 秒定时上报 + 20 秒语音间隔（最终版）

**当前逻辑（commit `d847eb0`）：**

| 模式 | 触发方式 | 间隔 |
|------|----------|------|
| **LLM 开启** | 每 **30 秒**读取传感器并 `POST /plant` | 上一段 **播完后 ≥ 20 秒** 才播下一段 |
| **LLM 关闭** | 不调服务器，只播待机预录 | 上一段 **播完后 ≥ 20 秒** 一条 |

**要点：**
- `SERVER_INTERVAL_MS = 30000`：定时尝试上报传感器数据
- `SPEECH_INTERVAL_MS = 20000`：任意两段语音之间的最小静音（从**播完**起算，不是从开播）
- `runPeriodicServerUpdate()`：30 秒到点且满足 20 秒静音 → 调 LLM；服务器失败 → 改播预录
- **已移除**传感器变化立即触发 LLM 的路径（原 `isDemoTriggerReady` / `runDemoModePost`）
- TTS 等待/播放期间仍与预录**互斥**（`demoAudioCycleActive` / `mp3Playing`）

**串口验证（LLM 开）：**
```
Periodic mode: posting sensor data to server.
Waiting for server TTS...
```

**串口验证（LLM 关）：**
```
Idle mode: library clip started.
MP3 downloaded to RAM: ...
```

**时间线示例（LLM 开）：** 30s 第一次 POST → TTS 播约 5s → 播完后再静音 20s → 下一个 30s 到点且已满 20s 才第二次 POST。实际间隔通常为 `max(30s, 20s + 上一段时长)`。

### 2.3 预录 RAM 播放与 TTS 互斥

**期望逻辑：**
- **定时 LLM**（30s）：`POST /plant` → TTS（`latest.mp3`）
- **待机预录**（LLM 关或服务器失败）：每 20 秒播一条预录库
- 两种声音互斥，不可叠加

**实现要点：**
- `demoAudioCycleActive`：TTS 等待/播放期间禁止预录
- `markAudioPlaybackFinished()`：播完重置 `lastIdleAudioTime`
- `canPlayIdleLibraryAudio()` / `runIdleLibraryAudio()`：LLM 关闭时的 20 秒预录

### 2.4 传感器阈值调整

**问题：** 原阈值偏低，光照/运动/声音边界容易抖动。

**调整（与 GitHub 远程合并后保留）：**

| 参数 | 原值 | 当前值 | 说明 |
|------|------|--------|------|
| `ACTIVE_THRESHOLD` | 0.06 | **0.10** | 运动判定更不敏感 |
| `ACTIVE_HOLD_TIME_MS` | 1500 | **2200** | 减少 still/active 抖动 |
| `WALK_ENTER_LEVEL` | 0.060 | **0.095** | 散步态需更明显晃动 |
| `LUX_WILTED_MAX` | 50 | **30** | 需更暗才算蔫 |
| `LUX_NEED_SUN_MAX` | 300 | **450** | 室内亮灯不易跳 sunlight |
| `LUX_OUTSIDE_HINT` | 800 | **1500** | 与远程仓库一致，减少误判室外 |
| 声音 quiet/active 阈值 | 较低 | **略提高** | 减少 place 闪烁 |

> 注：当前版本**不再**用传感器变化触发 LLM，上述阈值主要影响状态显示与预录分类前缀。

### 2.5 网页端 LLM 开关同步（硬件侧）

- 新增 `refreshLlmDemoSetting()`：约每 **10 秒** 读 `GET /latest` 中的 `llm_enabled`
- **开启**：走 30 秒定时 `runPeriodicServerUpdate()`
- **关闭**：跳过服务器，走 `runIdleLibraryAudio()` 每 20 秒预录
- 关闭时：取消进行中的 TTS 等待

**需重新烧录固件后生效。**

### 2.6 其他固件修复

- 补充 `showTextOnOLED` 等前向声明，修复编译错误
- 合并远程仓库时保留 OLED 英文 fallback（`getOLEDStatusFallback`）、扬声器音量 `120`

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
2. 设备平放即可，无需刻意遮光/摇晃
3. 约 **20 秒**一条待机预录；串口见 `Idle mode: library clip started`

### 演示 AI 对话（30 秒定时）

1. **打开「大模型对话」**（硬件约 10 秒内同步）
2. 设备会自动每 **30 秒**上报当前传感器数据
3. 须等上一条 **播完后再静音 20 秒** 才会播下一条
4. 串口见 `Periodic mode: posting sensor data to server.`

### 语音互斥

- TTS 播放/等待期间不会插播预录
- 预录播放期间不会进入 LLM 等待

---

## 六、Git 提交记录（2026-05-31）

| Commit | 说明 |
|--------|------|
| `5bf9cb8` | 语音互斥、LLM 开关、Vue 设置页、预录 RAM 播放等 |
| `d847eb0` | 恢复 30s 定时上报；保留 20s 语音间隔；移除传感器即时触发 LLM |

**仓库：** https://github.com/ChiaraYang-art/AI-builders

---

## 七、主要改动文件清单

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
docs/CHANGELOG_2026-05-31.md
deploy/.env.example                  # TTS 音色（非 .env 密钥）
```

---

## 八、提交前自检清单

- [ ] 已烧录最新 `.ino`（含 `d847eb0`）到 AtomS3
- [ ] Flask 已重启，`GET /latest` 含 `llm_enabled`、`tts_voice=longhuhu_v3`
- [ ] Vue dev 可访问，「我的」页开关可切换
- [ ] LLM **关**：串口约每 20s 见 `Idle mode: library clip started`
- [ ] LLM **开**：串口约每 30s 见 `Periodic mode: posting sensor data to server.`
- [ ] 预录播放串口出现 `MP3 downloaded to RAM`
- [ ] 两段语音之间静音 ≥ 20 秒（从播完起算）

---

## 九、已知限制 / 后续可选

1. **实时 TTS 仍流式播放**，大段 `latest.mp3` 在硬件上可能略卡；可选改为与预录相同的 RAM 整段下载
2. **Vue 日记页** 部分仍为 demo 静态数据，未完全接入 `diary_history` API
3. 传感器阈值可按实际路演环境继续微调

---

*文档最后更新：2026-05-31（含 commit d847eb0）*
