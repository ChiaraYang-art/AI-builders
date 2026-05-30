# City Sprout 全链路验证文档

**对照 PRD**：`PRD_CitySprout_软硬件一体版_2026-05-28.md`  
**版本**：2026-05-30  
**适用范围**：硬件 Demo + Flask 后端 + Vue App（`08_vue_figma_strict_demo`）

---

## 1. 验证目标

证明以下闭环可用：

```text
硬件感知 → POST /plant → AI 文案 + TTS
                ↓
         GET /latest → Vue 首页同步
                ↓
    散步 walk/start → photo/audio → diary
                ↓
         日记 / 图鉴 / 地图 ← /latest
```

---

## 2. 一键自动化验证（推荐先做）

### 2.1 前置条件

| 项 | 要求 |
|----|------|
| Python 3.10+ | 已安装依赖（`pip install -r backend/requirements.txt` 或项目既有环境） |
| Flask 已启动 | `cd backend && python sprout_server.py` |
| curl / bash | Git Bash、WSL 或 macOS/Linux |
| 可选 | `backend/.env` 配置 `DASHSCOPE_API_KEY`（验证 TTS + MP3） |

### 2.2 运行命令

**方式 A：对已启动的 Flask 做 HTTP 验证（与硬件/Vue 同网环境）**

```bash
# 终端 1：启动后端
cd backend
python sprout_server.py

# 终端 2：一键验证（项目根目录）
chmod +x scripts/verify_city_sprout.sh   # 首次
./scripts/verify_city_sprout.sh
```

**方式 B：无需先启动 Flask（进程内 test_client，适合 CI / 本地快速验）**

```bash
./scripts/verify_city_sprout.sh --in-process --skip-tts-wait
```

**Windows（Git Bash / WSL）** 同样执行上述 bash 脚本。

**Windows（PowerShell，推荐）**

```powershell
cd AI-builders-main
.\scripts\verify_city_sprout.ps1 -InProcess          # 不依赖已启动 Flask，验最新代码
.\scripts\verify_city_sprout.ps1                     # 对已启动的 Flask 做 HTTP 验（需先重启后端）
.\scripts\verify_city_sprout.ps1 -SkipTtsWait
$env:CITY_SPROUT_BASE_URL="http://192.168.43.56:5000"; .\scripts\verify_city_sprout.ps1
```

> **重要**：若 HTTP 模式报 `/latest 缺少 active_walk`，说明当前 `python sprout_server.py` 是旧进程。在运行后端的终端 `Ctrl+C` 停掉后重新启动，再跑脚本。

仅测 API、不等 TTS：

```bash
./scripts/verify_city_sprout.sh --skip-tts-wait
```

连同 Vue 编译一起测：

```bash
./scripts/verify_city_sprout.sh --with-vue-build
```

局域网设备地址：

```bash
CITY_SPROUT_BASE_URL=http://192.168.43.56:5000 ./scripts/verify_city_sprout.sh
```

直接跑 Python（不经过 bash）：

```bash
cd backend
python verify_full_chain.py
python verify_full_chain.py --base-url http://127.0.0.1:5000
```

### 2.3 自动化覆盖范围

| 步骤 | PRD 章节 | 接口 | 优先级 |
|------|----------|------|--------|
| 服务可达 | 18.1 | `GET /latest` | P0 |
| 硬件/APP 上报 | 18.2 | `POST /plant` JSON + text/plain | P0 |
| 文案同步 | 6.2 | `/latest` 与 `/plant` 一致 | P0 |
| TTS 音频 | 17、18.2 | `tts_status=ready`、`GET /audio/latest.mp3` | P1 |
| Color Walk | 9、18.3 | `POST /walk/start` + `/walk/photo` | P0 |
| 照片访问 | 18.3 | `GET /walk/media/...` | P2 |
| Sound Walk | 10、18.4 | `POST /walk/audio` | P1 |
| 散步日记 | 12、18.5 | `POST /walk/diary` | P1 |
| 图鉴解锁 | 15 | `/latest.atlas_unlocked` | P1 |
| Light / Local | 8、11 | `POST /walk/start` | P1 |

脚本退出码：`0` = 通过，`1` = 失败。

---

## 3. 软硬件联调验收（推荐顺序）

按顺序做，可在一小时内完成 P0+P1 全链路签字。

| 步骤 | 操作 | 通过标准 |
|------|------|----------|
| ① 代码自动化 | `.\scripts\verify_city_sprout.ps1 -InProcess` | 56 项全 PASS |
| ② 重启后端 | 停旧进程 → `cd backend` → `python sprout_server.py` | 日志显示 `0.0.0.0:5000` 与本机热点 IP |
| ③ HTTP 复验 | `.\scripts\verify_city_sprout.ps1` | 无 `active_walk` 缺失 |
| ④ 硬件配置 | `arduino_secrets.h` 中 `SECRET_SERVER_URL` = `http://<电脑IP>:5000/plant` | 与 Flask 终端 IP 一致 |
| ⑤ 硬件联调 | 烧录 `city_sprout_pahub_main_v4_no_flicker_canvas` | 见下方 4.1 / 4.2 |
| ⑥ Vue 联调 | `cd app_demo/08_vue_figma_strict_demo` → `npm run dev` | 见下方 4.2 |
| ⑦ 签字表 | 本文第 7 节 | P0 全勾后可 Demo |

**当前环境参考**（以你机器实测为准）：Flask `http://192.168.43.56:5000`，硬件 `192.168.43.41` 已成功 `POST /plant` 与 TTS。

---

## 4. 人工验收清单（自动化无法替代）

自动化通过后，按下列项人工确认。

### 4.1 Priority 0（Demo 必过）

- [ ] **硬件状态上传**：串口每 ~20s 出现 `Send to server`，无 `HTTP failed -1`
- [ ] **AI 文案**：`Plant says:` 为中文（非 `It is quiet...` 兜底）
- [ ] **OLED**：显示状态行 + 短句，不频繁整屏闪烁
- [ ] **Vue 首页**：`http://localhost:5173/#/home` 显示 lux、温湿度、speech_full
- [ ] **Color Walk**：邀请 → 出门 → 拍照 → 进度增加

### 4.2 Priority 1（建议完成）

- [ ] **Voice Base TTS**：POST 成功后自动播放 MP3，播完恢复麦克风
- [ ] **Sound Walk**：浏览器录音上传，后端返回 sounds + summary
- [ ] **完成散步**：暂停页 → 生成日记 → 完成页 / 流水账 / 小作文有内容
- [ ] **图鉴**：Sound / Bloom 等项变为「已解锁」
- [ ] **地图**：显示 `map_summary`

### 4.3 Priority 2 / 画饼（本次不阻塞）

- [ ] 硬件实时声音内容识别（PRD 20：下版本）
- [ ] 真实地图轨迹 / Nearby 推荐算法
- [ ] 用户账号与多设备同步

---

## 5. 分模块验证步骤

### 4.1 后端单机（无硬件）

```powershell
cd backend
python sprout_server.py
```

另开终端：

```powershell
Invoke-RestMethod http://127.0.0.1:5000/latest
Invoke-RestMethod -Method POST http://127.0.0.1:5000/plant -ContentType "application/json" -Body '{"state":"sunlight","lux":580,"motion":"walking","sound_state":"quiet"}'
```

浏览器打开 `http://127.0.0.1:5000/`，确认可播放 MP3。

### 4.2 硬件链路

1. 烧录 `arduino/city_sprout_pahub_main_v4_no_flicker_canvas/`
2. `arduino_secrets.h` 中 `SECRET_SERVER_URL` 指向电脑热点 IP
3. 串口 115200 观察：
   - `Mic warmup attempt ... peak>0`
   - `WiFi connected`
   - `MP3 playback started` / `Mic restored after TTS playback`

### 4.3 Vue App

```powershell
cd app_demo/08_vue_figma_strict_demo
npm run dev
```

| 页面 | 验证点 |
|------|--------|
| 首页 | 4s 内数据更新；点「开启语音」可播 MP3 |
| 邀请页 | 点「出门！」→ `POST /walk/start` |
| Color Walk | 拍照上传，网格预览 |
| Sound Walk | 录音上传 |
| 暂停页 | 「完成散步」→ 日记生成 |
| 日记 / 地图 / 图鉴 | 读 `/latest.walk_diary`、`atlas_unlocked` |

---

## 6. 预期输出示例

### 5.1 脚本成功

```text
[PASS] P0: GET /latest 服务可达
[PASS] P0: POST /plant (JSON)
[PASS] P0: POST /plant (Arduino text/plain)
...
通过: 28  失败: 0  警告: 0
结果: 全部自动化项通过
```

### 5.2 常见失败与处理

| 现象 | 原因 | 处理 |
|------|------|------|
| 无法连接 /latest | Flask 未启动 | `python sprout_server.py` |
| TTS 超时 | 无 Key / 百炼未开通 | 配置 `.env` 或 `--skip-tts-wait` |
| /walk/photo 400 | walk_id 无效 | 先 `/walk/start` |
| 硬件 HTTP -1 | IP 或 WiFi 不对 | 更新 `arduino_secrets.h` |

---

## 7. 文件索引

| 文件 | 说明 |
|------|------|
| `scripts/verify_city_sprout.sh` | 一键 bash 入口 |
| `scripts/verify_city_sprout.ps1` | 一键 PowerShell 入口（Windows） |
| `backend/verify_full_chain.py` | HTTP 全链路断言 |
| `backend/test_dialogue_api.py` | Flask test_client 对话测试（可选 `--with-dialogue-test`） |
| `PRD_CitySprout_软硬件一体版_2026-05-28.md` | 产品需求原文 |

---

## 8. 验收签字表（Demo 前）

| 模块 | 自动化 | 人工 | 负责人 | 日期 |
|------|--------|------|--------|------|
| POST /plant + /latest | ☐ | ☐ | | |
| TTS + MP3 | ☐ | ☐ | | |
| Color Walk | ☐ | ☐ | | |
| Sound Walk | ☐ | ☐ | | |
| 散步日记 + 图鉴 | ☐ | ☐ | | |
| 硬件 Voice Base | ☐ | ☐ | | |
| Vue 首页同步 | ☐ | ☐ | | |

全部 P0 通过后，即可进行 6 月 Demo 演示。
