# 出走小芽后端

Flask 服务：接收硬件传感器、用 LangChain + 百炼 `qwen-plus` 生成小芽对话文案，并可选 TTS 语音合成。

> **Legacy 副本**：重构前的扁平结构完整备份在 [`../backend_legacy/`](../backend_legacy/)，行为应与本文档描述一致，仅代码组织不同。

## 目录结构

```text
backend/
├── app.py              # Flask 入口（推荐）
├── sprout_server.py    # 兼容入口（gunicorn / 旧文档）
├── config.py           # 环境变量与路径
├── api/                # HTTP 路由
├── ai/                 # LangChain / 百炼调用
├── utils/              # 状态解析、内存存储
├── prompts/            # Prompt 模板
├── tests/              # 接口与全链路测试
│   ├── test_dialogue_api.py
│   └── verify_full_chain.py
└── generated/          # 运行时 MP3 / 散步媒体（勿提交）
```

## 快速启动

```bash
export DASHSCOPE_API_KEY="你的百炼 API Key"
cd backend
python app.py
```

默认监听 `http://0.0.0.0:5000`。若 5000 被占用（macOS 常见为 AirPlay），可改用其他端口：

```bash
SPROUT_PORT=5001 python app.py
```

## 环境变量

| 变量 | 说明 | 默认值 |
|------|------|--------|
| `DASHSCOPE_API_KEY` | 百炼 API Key（LLM + TTS） | 无，未配置时 LLM 走规则兜底 |
| `DASHSCOPE_LLM_MODEL` | 对话模型 | `qwen-plus` |
| `DASHSCOPE_VLM_MODEL` | 识图模型 | `qwen-vl-plus` |
| `DASHSCOPE_WS_URL` | TTS WebSocket 地址 | 北京地域默认 URL |
| `DASHSCOPE_TTS_MODEL` | TTS 模型 | `cosyvoice-v3-flash` |
| `DASHSCOPE_TTS_VOICE` | TTS 音色 | `longanyang` |
| `SPROUT_PORT` | 本地监听端口 | `5000` |

密钥勿提交到仓库，部署时使用环境变量或 `deploy/.env`。

## AI 对话接口

| 接口 | 说明 |
|------|------|
| `POST /plant` | 上传传感器状态，生成小芽文案 |
| `GET /latest` | APP 轮询最新状态与文案 |

`POST /plant` 响应方式：

- 默认：`text/plain`，返回 `speech_short`（兼容 Arduino）
- `Accept: application/json`：返回 JSON（含 `speech_short`、`speech_full`、`state`）

Prompt 定义见 [`prompts/sprout_speech.md`](prompts/sprout_speech.md)。

## 散步接口

| 接口 | 说明 |
|------|------|
| `POST /walk/start` | 开始散步会话 |
| `POST /walk/photo` | Color Walk 上传照片 |
| `POST /walk/audio` | Sound Walk 上传录音 |
| `POST /walk/diary` | 生成散步日记 |
| `GET /walk/media/<walk_id>/<file>` | 获取散步媒体 |

## 本地测试

从 `deploy/.env` 读取配置，支持两种模式：

| TEST_MODE | 说明 |
|-----------|------|
| `local`（默认） | Flask 内存测试，使用 `DASHSCOPE_API_KEY` 调用 qwen-plus |
| `remote` | HTTP 请求 `SPROUT_SERVER_URL` 指向的已部署服务 |

```bash
cd backend
python -m tests.test_dialogue_api
python -m tests.verify_full_chain --in-process
```

示例 curl：

```bash
curl -X POST http://127.0.0.1:5000/plant \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -d '{"state":"need_sun","lux":32.5,"motion":"still"}'

curl http://127.0.0.1:5000/latest
```

## 硬件联调

Arduino 中不要使用 `127.0.0.1`，应使用 Flask 启动时打印的局域网地址，例如：

```text
http://192.168.x.x:5000/plant
```

## 浏览器语音页

```text
http://<服务器地址>:5000/
```

播放前需点击「Enable Audio」；浏览器会拦截未交互的自动播放。

生产依赖见 `deploy/requirements-prod.txt`，Docker 部署见 `deploy/README.md`。
