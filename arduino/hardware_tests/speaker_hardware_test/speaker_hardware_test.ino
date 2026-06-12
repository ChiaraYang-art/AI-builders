#include <Arduino.h>
#include <M5Unified.h>
#include <math.h>

/*
  City Sprout speaker hardware test.

  Purpose:
  - No WiFi.
  - No Flask backend.
  - No MP3 decoding.
  - No API.
  - Only tests whether AtomS3R + Atomic Voice Base / speaker can make sound.

  Behavior:
  - On boot, plays a simple tone scale.
  - Press BtnA to replay the test.
  - Serial monitor prints whether M5.Speaker reports enabled.
*/

const int SPEAKER_VOLUME = 200;
const int SAMPLE_RATE = 16000;
const int TONE_MS = 300;

void drawStatus(const char* line1, const char* line2) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(6, 10);
  M5.Display.println("Speaker Test");
  M5.Display.setCursor(6, 34);
  M5.Display.println(line1);
  M5.Display.setCursor(6, 52);
  M5.Display.println(line2);
  M5.Display.setCursor(6, 92);
  M5.Display.println("BtnA: replay");
}

void playToneBlocking(int frequency, int durationMs, int gapMs = 80) {
  Serial.print("tone ");
  Serial.print(frequency);
  Serial.println(" Hz");

  M5.Speaker.tone(frequency, durationMs);
  delay(durationMs + gapMs);
  M5.Speaker.stop();
}

void playRawSine(int frequency, int durationMs) {
  const int samples = SAMPLE_RATE * durationMs / 1000;
  const int chunkSamples = 256;
  int16_t buffer[chunkSamples];

  Serial.print("raw sine ");
  Serial.print(frequency);
  Serial.println(" Hz");

  int generated = 0;
  while (generated < samples) {
    int n = min(chunkSamples, samples - generated);

    for (int i = 0; i < n; i++) {
      float t = (float)(generated + i) / SAMPLE_RATE;
      float wave = sinf(2.0f * PI * frequency * t);
      buffer[i] = (int16_t)(wave * 12000);
    }

    M5.Speaker.playRaw(buffer, n, SAMPLE_RATE, false, 1, 0);
    generated += n;
    delay(1);
  }

  delay(80);
  M5.Speaker.stop();
}

void runSpeakerTest() {
  drawStatus("Playing tones...", "Listen closely");

  Serial.println("=== speaker test begin ===");
  Serial.print("Speaker enabled before begin: ");
  Serial.println(M5.Speaker.isEnabled() ? "true" : "false");

  if (!M5.Speaker.isEnabled()) {
    M5.Speaker.begin();
  }

  M5.Speaker.setVolume(SPEAKER_VOLUME);

  Serial.print("Speaker enabled after begin: ");
  Serial.println(M5.Speaker.isEnabled() ? "true" : "false");

  playToneBlocking(440, 220);
  playToneBlocking(660, 220);
  playToneBlocking(880, 280);

  playRawSine(523, TONE_MS);
  playRawSine(784, TONE_MS);

  drawStatus("Done.", "If silent: hardware/config");
  Serial.println("=== speaker test end ===");
}

void setup() {
  Serial.begin(115200);
  delay(800);

  auto cfg = M5.config();

  // Enable both common external speaker profiles for Atomic Voice Base tests.
  cfg.external_speaker.atomic_spk = true;
  cfg.external_speaker.atomic_echo = true;

  M5.begin(cfg);
  M5.Display.setRotation(3);
  M5.Display.setBrightness(120);

  auto speakerConfig = M5.Speaker.config();
  speakerConfig.sample_rate = SAMPLE_RATE;
  M5.Speaker.config(speakerConfig);
  M5.Speaker.begin();
  M5.Speaker.setVolume(SPEAKER_VOLUME);

  drawStatus("Booted.", "Starting test...");
  delay(500);
  runSpeakerTest();
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    runSpeakerTest();
  }

  delay(10);
}
