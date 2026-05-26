# City Sprout / 出走小芽

这是一个 AI + 硬件课程 Demo。

项目目标：一个可携带的小植物“小芽”通过光照和移动状态感知自己是否长期待在室内，并用植物口吻邀请用户带它出门。

当前核心链路：

```text
DLight 光照传感器
-> AtomS3R 判断植物状态
-> AtomS3R 彩屏显示小芽动画
-> Flask 后端生成植物语言
-> 浏览器可用电脑扬声器模拟 TTS
```

## 当前应该使用的代码

### 1. 视觉演示模式

文件：

```text
arduino/dual_screen_state_loop/dual_screen_state_loop.ino
```

用途：

- 只接 OLED，不接 DLight。
- AtomS3R 自带彩屏显示彩色小芽动画。
- 外接 OLED 横屏显示状态和植物短句。
- 状态每 5 秒自动轮播。

适合：

- 分线器没到时，单独演示双屏视觉效果。
- 检查 OLED 横屏文字显示是否正常。

### 2. 传感器 + 移动 + Flask 主程序

文件：

```text
arduino/dlight_flask_serial/dlight_flask_serial.ino
```

用途：

- 只接 DLight，不接 OLED。
- 读取真实 lux 光照值。
- 使用 AtomS3R 内置 IMU 判断设备是否被拿起或移动。
- 光照控制小芽姿态，移动控制小芽摆动。
- 每隔几秒把 `state / lux / motion` 发给 Flask 后端。
- Serial Monitor 打印后端返回的植物语言。

适合：

- 当前最主要的传感器链路测试。
- 展示“光照 + 移动 -> 小芽状态 -> 后端植物语言”。

### 3. 声音环境变化实验版

文件：

```text
arduino/dlight_imu_sound_flask/dlight_imu_sound_flask.ino
```

用途：

- 从当前主程序复制出来的实验版。
- 保留 DLight 光照、IMU 移动、Flask 后端。
- 新增麦克风声音环境变化检测。
- 判断声音是 `stable` 还是 `dynamic`。
- 用“光照充足 / 正在移动 / 声音变化大”三个条件判断 `indoor / outside / unknown`。

注意：

- 这个版本只检测声音环境变化，不识别声音内容。
- 第一次上传后需要看 Serial Monitor 里的 `sound level / range / var`，再调整声音阈值。
- 如果麦克风没有初始化成功，`sound_state` 会显示 `unknown`。

### 4. Flask 后端

文件：

```text
backend/sprout_server.py
```

用途：

- 提供 `POST /plant` 接口。
- 接收 Arduino 发来的 `state / lux / motion`。
- 返回一句植物语言。
- 提供网页 TTS 页面，用电脑浏览器朗读最新植物语言。

运行：

```powershell
python backend/sprout_server.py
```

浏览器语音页面：

```text
http://127.0.0.1:5000/
```

## Arduino 上传前要改的配置

打开：

```text
arduino/dlight_flask_serial/dlight_flask_serial.ino
```

修改顶部这三行：

```cpp
const char* WIFI_SSID = "你的 Wi-Fi 名称";
const char* WIFI_PASSWORD = "你的 Wi-Fi 密码";
const char* SERVER_URL = "http://你的电脑局域网IP:5000/plant";
```

注意：

```text
SERVER_URL 不能写 127.0.0.1。
AtomS3R 必须使用电脑的局域网 IP，例如 192.168.10.36。
```

## 归档文件

旧测试代码已经移动到：

```text
ArchivedFiles/legacy_2026-05-26/
```

这些文件不是当前主流程，但保留作历史参考。

## 文档

```text
docs/wiring.md
docs/handoff.md
backend/README_backend.md
```
