# City Sprout 决赛前硬件更新（2026-06-03）

本文档汇总 2026-06-03 晚间对硬件固件的改动，面向 **6 月 5 日决赛** 路演准备：缩短语音间隔、提高播报音量、新增 v6 成长循环 Demo，同时 **保留 v5 原有 6 段路演循环**。

---

## 一、改进概览

| 类别 | 主要成果 |
|------|----------|
| v5 微调 | 语音上报间隔 30s → **10s**；TTS 音量 120 → **180**；6 段路演循环逻辑不变 |
| v6 新增 | **40s 成长循环**（4 × 10s）；彩色屏等比缩放成长；OLED 成长值 + 进度条；**插电即从 0s 开始** |
| 版本策略 | v5 = 传感器场景路演；v6 = 小芽长大故事线，两版独立目录，互不影响 |

---

## 二、v5：原路演 Demo（保持不变）

**路径：** `arduino/city_sprout_pahub_main_v5_roadshow_demo/city_sprout_pahub_main_v5_roadshow_demo.ino`

### 2.1 仍为 6 段自动循环（约 47 秒）

```text
idle → dark → need sun → walk → sunlight → city sound
```

各段时长：7s + 7s + 8s + 9s + 9s + 7s。

进入方式：串口发 `demo` 或 `auto`（与此前一致）。

### 2.2 本次仅调整的两处参数

| 参数 | 原值 | 新值 | 说明 |
|------|------|------|------|
| `SERVER_INTERVAL_MS` | 30000 | **10000** | 一句 TTS 播完后约 10 秒再请求下一句 |
| `SPEECH_INTERVAL_MS` | 30000 | **10000** | 待机预录语音的最小间隔（与上报节奏对齐） |
| `M5.Speaker.setVolume()` | 120 | **180** | Voice Base 外放音量提高，现场更易听清 |

### 2.3 适用场景

- 展示 **光照 / 移动 / 声音** 多传感器场景切换
- 展示 wilted、need sun、walking、city sound 等完整状态叙事

---

## 三、v6：成长循环 Demo（新增）

**路径：** `arduino/city_sprout_pahub_main_v6_growth_demo/city_sprout_pahub_main_v6_growth_demo.ino`

**配置模板：** `arduino/city_sprout_pahub_main_v6_growth_demo/arduino_secrets.example.h`  
（本地复制为 `arduino_secrets.h`，可从 v5 目录直接拷贝已有 secrets。）

### 3.1 40 秒成长循环（4 × 10 秒）

| 时间 | 档位 | 成长值 | 彩色屏视觉 |
|------|------|--------|------------|
| 0–10s | 1 芽 | 12 | scale **0.45**，中间 1 个小白点花苞，茎矮，无眼睛 |
| 10–20s | 2 苗 | 37 | scale **0.65**，3 个白花苞略大，出现眼睛 |
| 20–30s | 3 含苞 | 62 | scale **0.82**，1 朵淡粉 + 2 个白花苞 |
| 30–40s | 4 盛开 | 87 | scale **1.0**，3 朵粉花 + 太阳 |

四档均 **复用 v5 原有绘制逻辑**（茎干、`drawPinkFlower`、眼睛），只做等比缩放与花苞数量差异，无需新素材。

### 3.2 OLED 显示

- 标题：`TINY BUD` / `SPROUT` / `BUDDING` / `BLOOM`
- 底部：`GROW xx/100` + 进度条
- 中间保留 AI 短句（3 行小字换行）

### 3.3 插电即演示

- `ROADSHOW_AUTO_ON_BOOT = true`
- Wi-Fi 连接期间固定显示第 1 档小芽
- **`setup()` 全部完成后** 调用 `startGrowthLoopTimer()`，从 **0 秒** 开始计时，避免 Wi-Fi 阻塞导致直接从第 2 档起步
- Demo 模式下跳过真实传感器读写，保证循环稳定
- **无需串口命令** 即可循环演示

### 3.4 与 v5 相同的音频参数

- `SERVER_INTERVAL_MS = 10000`
- `M5.Speaker.setVolume(180)`

### 3.5 适用场景

- 3 分钟路演中讲 **「小芽逐渐长大」** 产品故事
- 强调成长值、进度条等可视化

---

## 四、烧录与联调

### 4.1 选哪个版本？

| 目的 | 烧录目录 |
|------|----------|
| 完整传感器场景路演 | `city_sprout_pahub_main_v5_roadshow_demo` |
| 小芽成长故事（插电即播） | `city_sprout_pahub_main_v6_growth_demo` |

### 4.2 secrets 配置

两版均使用本地 `arduino_secrets.h`（勿提交 Git）。v6 首次使用：

```text
复制 v5 目录下的 arduino_secrets.h → v6 目录
```

或参考 `arduino_secrets.example.h` 填写 Wi-Fi 与 `SECRET_SERVER_URL`（须为局域网 IP，不能用 127.0.0.1）。

### 4.3 决赛前自检清单

- [ ] 所选版本烧录成功，彩屏 / OLED 均有画面
- [ ] Wi-Fi 连上，TTS 或预录 fallback 能出声
- [ ] v5：串口 `demo` 后 6 段场景按序切换
- [ ] v6：断电重插，从最小花芽开始，约每 10 秒长大一档
- [ ] 现场音量在 180 下是否足够（可按场地再微调）

---

## 五、串口命令（两版通用，可选）

波特率 **115200**。v6 默认不需串口；调试时仍可用：

| 命令 | 作用 |
|------|------|
| `demo` / `auto` | 进入自动循环（v6 会从 0s 重新计时） |
| `live` | 切回真实传感器 |
| `idle` / `dark` / `need` / `sun` / `walk` / `city` | 强制单场景 |
| `help` | 打印命令列表 |

---

## 六、后续可做（本次未做）

- 成长值由真实传感器（光 + 移动 + 声音变化）累计，而非 Demo 自动跳档
- NVS 持久化成长值与用户散步偏好
- 与后端 `growth_delta`、APP 图鉴页同步

详见 PRD 第 15.3 节；决赛 P0 仅完成硬件 Demo 可视化。

---

## 七、相关文件

```text
arduino/city_sprout_pahub_main_v5_roadshow_demo/
  city_sprout_pahub_main_v5_roadshow_demo.ino   # 6 段路演 + 10s/180 音量
  arduino_secrets.h                             # 本地，不提交

arduino/city_sprout_pahub_main_v6_growth_demo/
  city_sprout_pahub_main_v6_growth_demo.ino     # 4 段成长循环
  arduino_secrets.example.h
```
