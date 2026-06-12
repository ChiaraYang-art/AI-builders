# City Sprout Arduino

这个目录存放小芽硬件固件。当前主线保留在根部，历史实验和单项硬件测试已经拆分到 `archive/` 与 `hardware_tests/`。

## 当前主线

| 路径 | 用途 |
| --- | --- |
| `city_sprout_pahub_main_v5_roadshow_demo/` | 当前路演推荐固件，适合稳定展示传感器场景 |
| `city_sprout_pahub_main_v6_growth_demo/` | 小芽成长循环 Demo，适合讲述“照顾后长大”的故事线 |
| `city_sprout_pahub_main_v4_no_flicker_canvas/` | 完整联调主线之一，重点解决屏幕闪烁与多模块协同 |

## 硬件测试

`hardware_tests/` 里是单项排查用固件：

```text
hardware_tests/
  speaker_hardware_test/
  voice_base_mic_test/
  voice_base_mp3_http_test/
  voice_base_state_sound_test/
```

当主程序异常时，先用这些测试分别确认麦克风、扬声器、HTTP MP3 播放和状态音频链路。

## 历史版本

`archive/` 保存早期 OLED、DLight、IMU、Flask 串联、双屏轮播等实验版本。它们不再是当前主线，但有助于理解硬件方案从单模块测试到完整联调的演进。

## 本地密钥

需要联网的固件通常提供 `arduino_secrets.example.h`。使用前复制为 `arduino_secrets.h` 并填写本地 Wi-Fi、服务器地址等信息。`arduino_secrets.h` 已在 `.gitignore` 中忽略，不要提交到 GitHub。
