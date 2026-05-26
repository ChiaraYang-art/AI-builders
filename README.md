# City Sprout / 出走小芽

一个 AI + 硬件课程 Demo：可携带的小植物“小芽”通过光照、动作和环境数据感知自己是否长期待在室内，并用植物口吻邀请用户带它出门。

## 当前核心文件

```text
arduino/dual_screen_state_loop/dual_screen_state_loop.ino
arduino/dlight_flask_serial/dlight_flask_serial.ino
backend/sprout_server.py
```

## 模式 A：视觉演示

只连接 OLED。

```text
AtomS3R -> OLED Unit
```

打开并上传：

```text
arduino/dual_screen_state_loop/dual_screen_state_loop.ino
```

效果：

- AtomS3R 自带彩屏显示彩色小芽动画。
- 外接 OLED 横屏显示状态和植物短句。
- 状态每 5 秒自动轮播。

## 模式 B：传感器 + 后端

只连接 DLight。

```text
AtomS3R -> DLight Unit
```

先启动 Flask：

```powershell
python backend/sprout_server.py
```

再打开并上传：

```text
arduino/dlight_flask_serial/dlight_flask_serial.ino
```

上传前需要按现场网络修改 Arduino 文件顶部：

```cpp
const char* WIFI_SSID = "...";
const char* WIFI_PASSWORD = "...";
const char* SERVER_URL = "http://192.168.x.x:5000/plant";
```

## 浏览器语音模拟

运行后端后，电脑浏览器打开：

```text
http://127.0.0.1:5000/
```

点击 `Enable Voice` 后，浏览器会朗读最新植物文案，用电脑扬声器临时模拟 TTS。
