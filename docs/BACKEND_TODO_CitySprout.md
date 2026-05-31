# City Sprout 服务端待办事项

**对照 PRD**：[`PRD_CitySprout_软硬件一体版_2026-05-28.md`](../PRD_CitySprout_软硬件一体版_2026-05-28.md) 第 19 节（Priority 0 / 1）  
**代码目录**：`backend/`（分层结构见 [`agents.md`](../agents.md) 第 3 节）  
**更新日期**：2026-05-30

本文档仅列出**服务端**相对 PRD 尚未完成或仅部分完成的事项。  
`tests/verify_full_chain.py` 通过表示 HTTP 主链路可用，**不代表** PRD 语义已全部满足。

---

## 1. 总览

| 优先级 | 服务端相关项 | ✅ 已完成 | ⚠️ 部分完成 | ❌ 未完成 |
|--------|-------------|----------|------------|----------|
| **P0** | 7 项 | 5 | 1 | 1 |
| **P1** | 6 项 | 1 | 5 | 0 |

图例：

- **✅ 已完成**：接口与核心逻辑已就绪，APP/硬件可直接对接
- **⚠️ 部分完成**：有接口或兜底实现，但未达 PRD 章节描述的完整语义
- **❌ 未完成**：PRD 明确要求、当前代码缺失

---

## 2. 已实现能力（无需重复开发）

以下 P0/P1 服务端能力已在当前代码中存在，待办中不再重复：

| 能力 | 位置 |
|------|------|
| `POST /plant` 传感器上传 + 文案生成 | `api/plant.py`、`ai/speech.py` |
| `GET /latest` 状态轮询 | `api/latest.py` |
| `GET /audio/latest.mp3` TTS | `api/latest.py`、`ai/tts.py` |
| `POST /walk/start` / `photo` / `audio` / `diary` | `api/walk.py` |
| Color Walk 识图（qwen-vl-plus） | `ai/vision.py` |
| 散步日记生成（timeline + essay） | `ai/diary.py` |
| 图鉴解锁列表（完成 walk 后） | `utils/storage.py` → `/latest.atlas_unlocked` |
| 密钥与环境变量 | `config.py`、`deploy/.env.example` |

---

## 3. P0 待办（必须完成）

### 3.1 ❌ 小芽日记基础流水账（PRD 12.2）

**PRD 要求**：流水账记录散步过程中，小芽根据**传感器数据**生成并说出的话（`/plant` 触发的 speech），散步结束后可回看。

**当前缺口**：

- `api/plant.py` 只更新 `latest_message`，未写入 active walk 的 `events[]`
- `utils/storage.py` 的 `events` 目前仅有：`start` / `photo` / `audio`，缺少 `kind: "speech"`

**建议改法**：

1. 在 `utils/storage.py` 新增 `add_speech_event(walk_id, text, sensor_snapshot?)`
2. 在 `api/plant.py` 生成文案后，若存在 `_active_walk_id`，追加一条 speech 事件
3. `ai/diary.py` 的 `timeline` 将自动包含路上小芽说过的话

**验收**：散步期间多次 `POST /plant` → `GET /latest` 的 `active_walk.events` 含 speech 条目 → `POST /walk/diary` 的 `timeline` 可还原 PRD 12.2 示例。

---

### 3.2 ⚠️ Color Walk AI 识图字段（PRD 9.3）

**PRD 要求**：识别物体、主色、是否含植物/树/花/天空/街道、**是否符合当前任务**、收集进度。

**当前缺口**：

- 已有 `objects`、`colors`、`summary`、`color_progress`
- 缺少：`main_colors`（或 PRD 对齐命名）、`task_fit` / 任务符合度、植物/天空等结构化标签

**建议改法**：

1. 扩展 `ai/vision.py` 的 Prompt 与返回 JSON  schema
2. `api/walk.py` 的 `/walk/photo` 响应补充上述字段（可与 PRD 18.3 对齐）

**验收**：上传照片后响应含任务是否符合 Color Walk、主色列表与进度。

---

## 4. P1 待办（建议本次完成）

### 4.1 ⚠️ 真实 Sound Walk 音频识别（PRD 10.1、10.4、18.4）

**PRD 要求**：APP 上传 5–10 秒录音，识别鸟鸣、车流、人声、风声等。

**当前缺口**：

- `ai/audio.py` **未将音频文件**送入百炼 ASR / 音频理解模型
- 仅用 LLM 根据**文件名**推测场景标签

**建议改法**：

1. 在 `ai/audio.py` 接入百炼 Paraformer 或音频多模态 API，传入 `audio_path`
2. 保留现有 JSON 输出格式：`sounds[]`、`summary`

**验收**：上传真实 `.wav` 后，`sounds` 与录音内容相关，而非固定随机兜底。

---

### 4.2 ⚠️ 小作文日记输入维度（PRD 12.3、18.5）

**PRD 要求**：小作文输入包括光照变化、温湿度对比、散步时长、状态变化、任务完成情况等。

**当前缺口**：

- `ai/diary.py` 仅传入 `events/photos/audios/color_progress`
- walk session 未积累传感器时间序列

**建议改法**：

1. `utils/storage.py` 在 walk 期间记录传感器快照（或复用 speech events 中的 snapshot）
2. `generate_walk_diary()` 将这些字段写入 Prompt

**验收**：生成的 `essay` 能提及本次 walk 的光照/任务/时长等信息。

---

### 4.3 ⚠️ 小芽图鉴完整 API（PRD 15.1、15.2、15.3）

**PRD 要求**：展示已解锁 / 未解锁、解锁条件、下一种解锁条件、成长进度。

**当前缺口**：

- 仅有 `/latest.atlas_unlocked`（字符串列表）
- 无 `GET /atlas` 或等价结构
- 无成长统计（出门次数、照片数、日记数等，PRD 15.3）

**建议改法**：

1. 新增 `api/atlas.py`（或扩展 `/latest`）
2. 在 `utils/storage.py` 维护图鉴定义表 + 解锁状态 + 简单计数器

**验收**：APP 可渲染「未解锁卡片 + 解锁条件文案 + 已解锁数量」。

---

### 4.4 ⚠️ 散步地图结构化点位（PRD 13.2）

**PRD 要求**：静态/模拟地图上展示照片、声音、阳光、任务完成等点位。

**当前缺口**：

- 仅有 `map_summary` 一句文案（如 `1.8km · N 张照片`）
- 无 `map_points[]`（类型 + mock 坐标/序号）

**建议改法**：

1. 在 `finish_diary()` 或 `public_walk_view()` 中生成 `map_points`
2. 类型枚举：`photo` / `audio` / `sunlight` / `task` 等；坐标可用序号或 mock lat/lng

**验收**：`POST /walk/diary` 或 `GET /latest.walk_diary` 返回可绘制的点位数组。

---

### 4.5 ⚠️ Sound Walk 展示字段（PRD 10.5）

**PRD 要求**：展示录音时间、识别结果、小芽反馈。

**当前缺口**：

- `active_walk.audios[]` 无独立 `recorded_at`
- 时间仅存在于 `events` 中，APP 需自行关联

**建议改法**：

- `add_audio_result()` 为每条 audio 增加 `recorded_at`（ISO 或 `HH:MM`）

**验收**：APP 可直接渲染 Sound Walk 列表，无需解析 `events`。

---

## 5. 建议实现顺序

按 Demo 风险与 PRD Priority 排序：

| 顺序 | 事项 | 优先级 | 主要改动 |
|------|------|--------|----------|
| 1 | `/plant` 写入 walk 流水账 | P0 | `api/plant.py`、`utils/storage.py` |
| 2 | Color Walk 识图字段补全 | P0 | `ai/vision.py`、`api/walk.py` |
| 3 | 真实音频识别 | P1 | `ai/audio.py` |
| 4 | 日记输入维度 + 传感器轨迹 | P1 | `utils/storage.py`、`ai/diary.py` |
| 5 | 图鉴 API | P1 | `api/atlas.py`、`utils/storage.py` |
| 6 | 地图 map_points | P1 | `utils/storage.py`、`ai/diary.py` |
| 7 | Sound Walk recorded_at | P1 | `utils/storage.py` |

---

## 6. 非本次范围（PRD Priority 2 或 APP/硬件）

以下出现在 PRD 但**不在**当前服务端 P0/P1 待办内：

- 硬件端实时录音识别（PRD 10.2、P2）
- 真实地图轨迹 / 定位 SDK（PRD 13.3、P2）
- Nearby 真实平台联动（PRD 11、P2）
- 用户账号 / 多设备同步（PRD 16、P2）
- 日记日历视图（PRD 12.4，偏 APP）
- APP 页面 UI、硬件 OLED 显示（服务端仅提供 API）

---

## 7. 验证方式

每完成一项待办，建议至少跑：

```bash
cd backend
python -m tests.verify_full_chain --in-process --skip-tts-wait
```

针对单项可补充：

- 流水账：多次 `/plant` + 检查 `active_walk.events`
- 图鉴 / 地图：断言 `/latest` 或新接口 JSON 字段

远程服务器：

```bash
TEST_MODE=remote SPROUT_SERVER_URL=http://<服务器IP>:5000 python -m tests.test_dialogue_api
```

---

## 8. 相关文档

- PRD：[`PRD_CitySprout_软硬件一体版_2026-05-28.md`](../PRD_CitySprout_软硬件一体版_2026-05-28.md)
- 后端结构：[`agents.md`](../agents.md)
- 全链路验证：[`docs/VERIFICATION_CitySprout.md`](VERIFICATION_CitySprout.md)
- 部署：[`deploy/README.md`](../deploy/README.md)
