# City Sprout / 出走小芽

City Sprout 是一个软硬件一体的 AI 交互原型：一个可携带的小植物“小芽”通过光照、移动、声音等环境信号感知自己是否长期待在室内，并用植物口吻邀请用户出门散步、记录城市里的颜色、声音与附近好去处。

当前仓库已经完成路演所需的 P0 展示闭环：硬件传感器、Flask 后端、语音/文案生成、Vue 手机端 Demo、Figma 视觉资产、Arduino 主程序与验证文档都已经打通。历史版本保留在 `archive/` 目录，方便追溯，但默认阅读和开发请从当前版本开始。

## 快速入口

| 入口 | 路径 | 用途 |
| --- | --- | --- |
| 项目书 | `docs/00_project_overview/PROJECT_BOOK_CitySprout_2026-06-10.md` | 从前期构思、设计理念、制作流程、版本迭代到商业化路径的完整整理 |
| 产品规格 | `docs/00_project_overview/PRODUCT_SPEC_CitySprout_2026-06-10.md` | 当前产品范围、功能、交互与未来规划 |
| 原始 PRD | `docs/00_project_overview/PRD_CitySprout_软硬件一体版_2026-05-28.md` | 早期软硬件一体需求文档 |
| 文档索引 | `docs/README.md` | 所有文档的分类导航 |
| 手机扫码 Demo | `app_demo/09_vue_mobile_qr_demo/` | 当前最适合展示的移动端 Demo |
| 桌面/Figma 严格版 Demo | `app_demo/08_vue_figma_strict_demo/` | 视觉还原与开发联调用 Demo |
| Flask 后端 | `backend/` | 传感器状态、散步流程、AI 文案/语音接口 |
| Arduino 主程序 | `arduino/` | 当前固件、硬件测试与历史固件归档 |

## 当前代码结构

```text
AI builders/
  app_demo/                  Vue 前端 Demo；当前版本在 08 和 09，历史版本在 archive
  arduino/                   AtomS3R / PaHUB / DLight / Voice Base 固件
  assets/                    项目素材，例如 color walk 示例照片
  backend/                   Flask API、AI 模块、后端测试
  deploy/                    部署脚本与 Docker 配置
  docs/                      项目书、PRD、验证、交接、迭代记录、对话整理
  experiments/               Qwen、DLight 等探索性实验
  exports/                   OLED / AtomS3R 屏幕 SVG 导出
  scripts/                   全链路验证脚本
  tools/                     辅助导出工具
  VoiceGenerate/             预生成 TTS 音频与生成脚本
```

## 本地运行

后端：

```powershell
cd backend
python sprout_server.py
```

手机扫码 Demo：

```powershell
cd app_demo/09_vue_mobile_qr_demo
npm install
npm run dev:mobile
```

桌面/Figma 严格版 Demo：

```powershell
cd app_demo/08_vue_figma_strict_demo
npm install
npm run dev
```

全链路验证：

```powershell
.\scripts\verify_city_sprout.ps1 -InProcess -SkipTtsWait
```

更完整的验收步骤见 `docs/02_build_run_handoff/VERIFICATION_CitySprout.md`。

## 硬件主线

| 路径 | 用途 |
| --- | --- |
| `arduino/city_sprout_pahub_main_v5_roadshow_demo/` | 当前路演推荐主程序，使用分段场景展示硬件能力 |
| `arduino/city_sprout_pahub_main_v6_growth_demo/` | 小芽成长循环 Demo |
| `arduino/city_sprout_pahub_main_v4_no_flicker_canvas/` | 完整联调主线之一，强调屏幕刷新稳定性 |
| `arduino/hardware_tests/` | 麦克风、扬声器、Voice Base 等单项硬件排查 |
| `arduino/archive/` | 早期固件与废弃实验版本 |

首次烧录前，请复制对应目录下的 `arduino_secrets.example.h` 为 `arduino_secrets.h`，再填写本地 Wi-Fi 与服务地址。`arduino_secrets.h`、`.env` 等本地密钥文件不应提交到 GitHub。

## 文档阅读顺序

1. 先读 `docs/00_project_overview/PROJECT_BOOK_CitySprout_2026-06-10.md`，快速理解项目从概念到实现的完整脉络。
2. 再读 `docs/00_project_overview/PRODUCT_SPEC_CitySprout_2026-06-10.md`，确认当前功能、P0 完成情况和未来功能池。
3. 需要复现或验收时读 `docs/02_build_run_handoff/VERIFICATION_CitySprout.md`。
4. 需要理解历史迭代时读 `docs/03_changelog/` 和 `docs/04_process_notes/`。

## 商业化方向

当前最自然的商业化路径是把 Local Discovery 与本地生活服务平台、综合种草社区结合：用户仍然以“小芽带我发现城市”为主体验，商家可以以推荐点位、任务、路线或卡片的方式植入，但需要控制普通推荐与商业推荐比例。详细方案见项目书中的“商业化路径”章节。
