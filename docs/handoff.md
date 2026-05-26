# City Sprout / 出走小芽 Handoff

当前项目已经从“真实城市树感知”收敛为“可携带 AI 小芽 / 出门提醒器”。

第一阶段优先跑通：

```text
传感器数据
-> 植物状态
-> 植物语言
-> 屏幕表达
```

当前已整理的三个核心文件：

```text
arduino/dual_screen_state_loop/dual_screen_state_loop.ino
arduino/dlight_flask_serial/dlight_flask_serial.ino
backend/sprout_server.py
```

开发模式：

```text
模式 A：只接 OLED
用于视觉演示：S3R 彩屏小芽动画 + OLED 状态文案轮播。

模式 B：只接 DLight
用于传感器链路：读取 lux -> 判断状态 -> 请求 Flask -> 打印/显示文案。
```

等待 Unit Hub / PaHUB 到货后，再把两条链路合并成 `city_sprout_main.ino`。
