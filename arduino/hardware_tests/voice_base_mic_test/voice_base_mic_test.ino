/*
  City Sprout / 出走小芽
  Atomic Voice Base microphone test

  这个文件只测试麦克风，不读取 DLight，不连接 Wi-Fi，不请求 Flask。

  使用方法：
  1. AtomS3R 插在 Atomic Voice Base 上。
  2. 上传这个 sketch。
  3. 打开 Serial Monitor，波特率 115200。
  4. 对着麦克风说话、拍手、吹气，看 level / peak 是否变化。

  判断结果：
  - 如果看到 "M5.Mic is enabled."，并且 level 会变化，说明 M5Unified 可以直接读麦克风。
  - 如果看到 "M5.Mic is NOT enabled."，说明 Voice Base 需要专门的 ES8311 / I2S 初始化。
*/

#include <Arduino.h>
#include <M5Unified.h>
#include <math.h>

static const int SAMPLE_RATE = 16000;
static const int SAMPLE_COUNT = 256;

int16_t samples[SAMPLE_COUNT];
unsigned long lastPrintTime = 0;

float calcRmsLevel(const int16_t* data, int count) {
  double sumSquares = 0;

  for (int i = 0; i < count; i++) {
    float sample = data[i] / 32768.0f;
    sumSquares += sample * sample;
  }

  return sqrt(sumSquares / count);
}

int16_t calcPeak(const int16_t* data, int count) {
  int16_t peak = 0;

  for (int i = 0; i < count; i++) {
    int16_t value = abs(data[i]);
    if (value > peak) {
      peak = value;
    }
  }

  return peak;
}

void drawStatus(bool micEnabled, bool recorded, float level, int16_t peak) {
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.setTextSize(1);

  M5.Display.setCursor(4, 6);
  M5.Display.print("Voice Base Mic Test");

  M5.Display.setCursor(4, 24);
  M5.Display.print("mic: ");
  M5.Display.print(micEnabled ? "enabled" : "not ready");

  M5.Display.setCursor(4, 40);
  M5.Display.print("record: ");
  M5.Display.print(recorded ? "ok" : "fail");

  M5.Display.setCursor(4, 56);
  M5.Display.print("level: ");
  M5.Display.print(level, 5);

  M5.Display.setCursor(4, 72);
  M5.Display.print("peak: ");
  M5.Display.print(peak);

  int barWidth = (int)(level * 1200);
  if (barWidth > 120) {
    barWidth = 120;
  }
  if (barWidth < 0) {
    barWidth = 0;
  }

  M5.Display.drawRect(4, 92, 120, 12, WHITE);
  M5.Display.fillRect(5, 93, barWidth, 10, GREEN);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  auto cfg = M5.config();

  /*
    M5Unified 示例里这个配置用于 ATOMIC ECHO BASE。
    Atomic Voice Base 不一定完全一样，但先打开它做第一轮通用测试。
  */
  cfg.external_speaker.atomic_echo = true;

  M5.begin(cfg);
  M5.Display.setBrightness(120);

  /*
    麦克风和扬声器通常不能同时使用。
    这里先关闭扬声器，避免它占用音频硬件。
  */
  M5.Speaker.end();

  /*
    调整麦克风配置。
    这些参数来自 M5Unified 官方 Microphone / Mic_FFT 示例的简化版。
  */
  auto micCfg = M5.Mic.config();
  micCfg.sample_rate = SAMPLE_RATE;
  micCfg.dma_buf_count = 3;
  micCfg.dma_buf_len = SAMPLE_COUNT;
  micCfg.noise_filter_level = 0;
  micCfg.over_sampling = 1;
  micCfg.magnification = micCfg.use_adc ? 16 : 1;
  M5.Mic.config(micCfg);

  bool beginOk = M5.Mic.begin();

  Serial.println();
  Serial.println("Atomic Voice Base microphone test start");
  Serial.print("M5.Mic.begin(): ");
  Serial.println(beginOk ? "true" : "false");
  Serial.print("M5.Mic.isEnabled(): ");
  Serial.println(M5.Mic.isEnabled() ? "true" : "false");
  Serial.println("--------------------------------");

  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(4, 8);
  M5.Display.print("Mic test starting");
}

void loop() {
  M5.update();

  bool micEnabled = M5.Mic.isEnabled();
  bool recorded = false;
  float level = 0;
  int16_t peak = 0;

  if (micEnabled) {
    recorded = M5.Mic.record(samples, SAMPLE_COUNT, SAMPLE_RATE);

    if (recorded) {
      level = calcRmsLevel(samples, SAMPLE_COUNT);
      peak = calcPeak(samples, SAMPLE_COUNT);
    }
  }

  drawStatus(micEnabled, recorded, level, peak);

  if (millis() - lastPrintTime > 300) {
    Serial.print("micEnabled=");
    Serial.print(micEnabled ? "true" : "false");
    Serial.print(" recorded=");
    Serial.print(recorded ? "true" : "false");
    Serial.print(" level=");
    Serial.print(level, 6);
    Serial.print(" peak=");
    Serial.println(peak);

    if (!micEnabled) {
      Serial.println("M5.Mic is NOT enabled. Voice Base may need ES8311/I2S-specific code.");
    }

    lastPrintTime = millis();
  }

  delay(50);
}
