/*
  City Sprout / 出走小芽
  文件：voice_base_state_sound_test.ino

  这是“不接大模型版”的 Atomic Voice Base 发声测试。

  目的：
  1. 不调用 Flask。
  2. 不调用百炼。
  3. 不需要 PaHUB / DLight / OLED / ENV-Pro。
  4. 只测试 AtomS3R + Atomic Voice Base 能不能根据植物状态发出不同声音。

  当前测试方式：
  - 程序每 5 秒自动切换一个植物状态。
  - 每次状态变化时，Atomic Voice Base 会播放一个对应的短音效。
  - AtomS3R 自带彩屏会显示当前状态。

  为什么先做这个：
  语音播放链路比较复杂，涉及网络、TTS、MP3 解码、Speaker。
  这个文件先验证最基础的事情：Voice Base 的扬声器能不能被 Arduino 控制并发声。

  如果这个程序能响，后续再把 playStateSound() 合并进主程序：
  - STATE_WILTED：低沉下行音
  - STATE_NEED_SUN：轻轻呼唤音
  - STATE_SUNLIGHT：明亮上行音
  - STATE_WALKING：短促脚步音
  - STATE_IDLE：柔和待机音
*/

#include <Arduino.h>
#include <M5Unified.h>

enum PlantState {
  STATE_IDLE = 0,
  STATE_WILTED = 1,
  STATE_NEED_SUN = 2,
  STATE_SUNLIGHT = 3,
  STATE_WALKING = 4
};

const unsigned long STATE_DURATION_MS = 5000;
const int SPEAKER_VOLUME = 160;

PlantState currentState = STATE_IDLE;
PlantState lastState = (PlantState)(-1);
unsigned long lastStateChangeTime = 0;

String plantStateToTitle(PlantState state) {
  if (state == STATE_WILTED) return "Wilted";
  if (state == STATE_NEED_SUN) return "Need Sun";
  if (state == STATE_SUNLIGHT) return "Sunlight";
  if (state == STATE_WALKING) return "Walking";
  return "Idle";
}

PlantState getAutoLoopState() {
  int index = (millis() / STATE_DURATION_MS) % 5;

  if (index == 0) return STATE_IDLE;
  if (index == 1) return STATE_WILTED;
  if (index == 2) return STATE_NEED_SUN;
  if (index == 3) return STATE_SUNLIGHT;
  return STATE_WALKING;
}

void playToneBlocking(int frequency, int durationMs, int gapMs = 30) {
  /*
    播放一个短音。

    frequency：
    音高，单位是 Hz。数字越大，声音越尖。

    durationMs：
    这个音持续多久，单位是毫秒。

    gapMs：
    播完后停顿多久，避免两个音粘在一起。
  */
  M5.Speaker.tone(frequency, durationMs);
  delay(durationMs + gapMs);
}

void playStateSound(PlantState state) {
  /*
    根据植物状态播放不同音效。

    注意：
    这里用的是 tone，也就是简单电子音。
    它不是 TTS 语音，但足够测试 Voice Base 扬声器是否可用。
  */
  M5.Speaker.stop();
  M5.Speaker.setVolume(SPEAKER_VOLUME);

  if (state == STATE_IDLE) {
    // 待机：很轻的两声，像小芽在呼吸。
    playToneBlocking(660, 90, 60);
    playToneBlocking(740, 120, 20);
    return;
  }

  if (state == STATE_WILTED) {
    // 蔫了：下行音，听起来低落。
    playToneBlocking(520, 130, 40);
    playToneBlocking(440, 150, 40);
    playToneBlocking(330, 220, 20);
    return;
  }

  if (state == STATE_NEED_SUN) {
    // 想晒太阳：轻轻呼唤的重复音。
    playToneBlocking(620, 100, 60);
    playToneBlocking(620, 100, 60);
    playToneBlocking(780, 160, 20);
    return;
  }

  if (state == STATE_SUNLIGHT) {
    // 得到阳光：明亮上行音，像展开叶子。
    playToneBlocking(660, 90, 30);
    playToneBlocking(880, 90, 30);
    playToneBlocking(1040, 180, 20);
    return;
  }

  if (state == STATE_WALKING) {
    // 行走：短促节奏，像被带着走。
    playToneBlocking(700, 70, 35);
    playToneBlocking(540, 70, 35);
    playToneBlocking(700, 70, 35);
    playToneBlocking(540, 70, 20);
    return;
  }
}

void drawStateOnScreen(PlantState state) {
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(8, 16);
  M5.Display.println("Sprout");

  M5.Display.setTextSize(1);
  M5.Display.setCursor(8, 48);
  M5.Display.print("State: ");
  M5.Display.println(plantStateToTitle(state));

  M5.Display.setCursor(8, 68);
  M5.Display.println("Voice Base tone");

  M5.Display.setCursor(8, 88);
  M5.Display.println("Auto loop: 5s");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  auto cfg = M5.config();

  /*
    Atomic Voice Base / Atomic Speaker 属于外接音频底座。
    不同 M5Unified 版本可能识别字段略有差异，
    这里同时打开 atomic_spk 和 atomic_echo，提高兼容性。
  */
  cfg.external_speaker.atomic_spk = true;
  cfg.external_speaker.atomic_echo = true;

  M5.begin(cfg);
  M5.Display.setBrightness(120);

  auto speakerConfig = M5.Speaker.config();
  speakerConfig.sample_rate = 64000;
  M5.Speaker.config(speakerConfig);

  M5.Speaker.begin();
  M5.Speaker.setVolume(SPEAKER_VOLUME);

  Serial.println("City Sprout Voice Base state sound test started.");
  Serial.print("Speaker enabled: ");
  Serial.println(M5.Speaker.isEnabled() ? "true" : "false");

  currentState = getAutoLoopState();
  lastState = currentState;
  drawStateOnScreen(currentState);
  playStateSound(currentState);
}

void loop() {
  M5.update();

  currentState = getAutoLoopState();

  if (currentState != lastState) {
    lastState = currentState;
    lastStateChangeTime = millis();

    Serial.print("State changed: ");
    Serial.println(plantStateToTitle(currentState));

    drawStateOnScreen(currentState);
    playStateSound(currentState);
  }

  /*
    按一下 AtomS3R 正面的按钮，可以手动重播当前状态音效。
    这样现场测试时不用等 5 秒。
  */
  if (M5.BtnA.wasPressed()) {
    Serial.print("Replay sound: ");
    Serial.println(plantStateToTitle(currentState));
    playStateSound(currentState);
  }
}
