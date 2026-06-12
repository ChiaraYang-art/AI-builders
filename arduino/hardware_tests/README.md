# Hardware Tests

这里是硬件单项排查固件，适合在主程序联调前确认各模块是否独立工作。

| 路径 | 用途 |
| --- | --- |
| `speaker_hardware_test/` | 扬声器基础测试 |
| `voice_base_mic_test/` | Voice Base 麦克风测试 |
| `voice_base_mp3_http_test/` | 通过 HTTP 播放 MP3 的链路测试 |
| `voice_base_state_sound_test/` | 状态音频播放测试 |

如果主程序表现异常，建议先从这里拆开验证，再回到主线固件。
