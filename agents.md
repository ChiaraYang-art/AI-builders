M5stack硬件官方文档链接：https://docs.m5stack.com/zh_CN/start

阿里云百炼api文档：https://bailian.console.aliyun.com/cn-beijing?tab=doc#/doc

---

## 1. 项目背景和内容

### 项目一句话

**出走小芽（City Sprout）** 是一棵可携带的 AI 小植物。它通过硬件感知光照、动作、声音、温湿度等环境变化，并通过 APP、OLED、语音和日记，把这些感知转译成一棵小芽对真实世界的体验。用户不是被提醒运动，而是在带一棵想见阳光、想听城市、想收集颜色的小植物出门。

### 产品形态

项目由三部分组成，数据流为：

```text
硬件小芽 → 后端 AI 服务 → 手机 APP
```

| 模块 | 职责 |
|------|------|
| **硬件小芽** | 读取光照、移动、声音环境、温湿度等；在 AtomS3R 彩屏 / OLED 显示状态；通过 Voice Base 播放提示 |
| **后端 AI 服务** | 理解植物状态；调用大模型生成小芽文案；图片 / 声音识别；散步记录整理；日记生成；TTS 语音合成 |
| **手机 APP** | 同步小芽状态与 AI 文案；Color Walk / Sound Walk / Light Walk；散步地图与日记；小芽图鉴与设置 |

### 核心体验闭环

```text
硬件感知状态 → AI 生成小芽表达 → 硬件端显示 / 播放 → APP 同步显示
→ 用户带小芽完成任务 → APP 记录照片、声音、地点、传感器数据
→ AI 生成总结和日记 → 用户可分享到社交媒体
```

### 当前阶段

- **版本**：2026-05-28 软硬件一体版
- **阶段**：硬件 Demo + APP 原型同步推进
- **详细需求**：见 [`PRD_CitySprout_软硬件一体版_2026-05-28.md`](PRD_CitySprout_软硬件一体版_2026-05-28.md)

实现时优先打通 Priority 0 链路：硬件 `/plant` 上传 → 大模型生成文案 → APP `/latest` 展示 → Color Walk 拍照上传 → 基础散步日记。

---

## 2. 项目主要技术栈

### 硬件

| 组件 | 说明 |
|------|------|
| AtomS3R | 主控 + 内置 IMU + 彩屏 |
| Atomic Voice Base | 麦克风 / 扬声器 |
| PaHUB | I2C 分线器 |
| DLight | 光照传感器 |
| OLED | 黑白屏，显示短句 |
| ENV-Pro | 温湿度、气压、空气质量 |
| 固件 | Arduino（C++），Wi-Fi 上报 `/plant` |

### 后端

| 技术 | 说明 |
|------|------|
| Python 3 | 主语言 |
| Flask | HTTP 服务框架 |
| LangChain | 大模型调用封装 |
| 阿里云百炼 DashScope | LLM（`qwen-plus`）、TTS（`cosyvoice-v3-flash`）、视觉 / 音频识别 |
| 环境变量 | `DASHSCOPE_API_KEY` 等，见 `.env.example.txt` |

### 手机 APP

| 技术 | 说明 |
|------|------|
| Vue 3 + Vite | 前端框架与构建 |
| 静态原型 | `app_demo/` 下多版 HTML / Vue Demo，严格对齐 Figma 设计稿 |

### 部署与联调

- Docker / `deploy/` 用于生产部署
- 硬件端使用 `arduino_secrets.h`（本地，勿提交）；后端密钥走环境变量
- 硬件请求服务器须用局域网 IP，不能用 `127.0.0.1`

---

## 3. 后端文件结构设计

后端代码按职责拆分到不同目录：**接口定义**、**核心 AI 调用**、**工具方法**、**测试代码** 分离，便于前后端和硬件统一对接 PRD 第 18 节接口。

### 推荐目录结构

```text
backend/
├── app.py                  # Flask 应用入口，注册蓝图、加载配置
├── config.py               # 环境变量与全局配置
│
├── api/                    # 接口定义（路由层，只做参数校验与响应组装）
│   ├── __init__.py
│   ├── plant.py            # POST /plant — 硬件上传传感器状态
│   ├── latest.py           # GET /latest — APP 轮询最新状态与文案
│   ├── walk.py             # POST /walk/photo、/walk/audio、/walk/diary
│   └── health.py           # 健康检查等
│
├── ai/                     # 核心 AI 调用（LangChain + 百炼，不含 HTTP 逻辑）
│   ├── __init__.py
│   ├── speech.py           # 根据状态生成小芽文案（speech_short / speech_full）
│   ├── vision.py           # Color Walk 图片识别与颜色总结
│   ├── audio.py            # Sound Walk 音频识别与总结
│   ├── diary.py            # 散步日记（timeline + essay）生成
│   └── tts.py              # 百炼 TTS，生成 MP3
│
├── utils/                  # 工具方法（无业务 HTTP、无模型调用）
│   ├── __init__.py
│   ├── state.py            # 植物状态枚举、状态推断规则
│   ├── storage.py          # 最新状态内存缓存 / 文件读写
│   └── response.py         # 统一 JSON 响应格式
│
├── prompts/                # Prompt 模板（Markdown / JSON）
│   ├── sprout_speech.md
│   ├── color_walk.md
│   ├── sound_walk.md
│   └── diary.md
│
├── generated/              # 运行时生成文件（如 latest.mp3），勿提交
│
└── tests/                  # 测试代码
    ├── __init__.py
    ├── test_dialogue_api.py
    └── verify_full_chain.py
```

### 分层原则

| 目录 | 职责 | 不应包含 |
|------|------|----------|
| `api/` | 定义 HTTP 路由、解析请求、调用 `ai/` 与 `utils/`、返回 JSON | 直接写 Prompt、直接调 DashScope SDK |
| `ai/` | 封装 LangChain / 百炼调用，输入结构化上下文，输出结构化结果 | Flask `request` / `jsonify` |
| `utils/` | 纯函数：状态映射、缓存、格式化、公共常量 | 路由、模型 API |
| `tests/` | 接口集成测试、AI 模块单元测试 | 生产业务逻辑 |

### 主要接口（与 PRD 对齐）

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/latest` | APP 获取小芽当前状态与 AI 文案 |
| POST | `/plant` | 硬件上传传感器数据，返回小芽话语 |
| POST | `/walk/photo` | Color Walk 照片上传与分析 |
| POST | `/walk/audio` | Sound Walk 录音上传与识别 |
| POST | `/walk/diary` | 根据 walk_id 生成散步日记 |

字段命名以 PRD 第 18 节为准；`api/` 层负责保持与硬件、APP 契约一致。

### 当前代码与迁移说明

- **主开发目录**：`backend/`（分层结构，见上文目录树）
- **Legacy 副本**：`backend_legacy/`（扁平结构完整备份，便于对照与回滚）
- 兼容入口：`sprout_server.py` 转发至 `app.py`（gunicorn / 旧文档）

本地测试：

```bash
cd backend
python -m tests.test_dialogue_api
python -m tests.verify_full_chain --in-process
```
