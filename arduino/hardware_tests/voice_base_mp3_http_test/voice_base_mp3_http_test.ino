/*
  City Sprout / 出走小芽
  文件：voice_base_mp3_http_test.ino

  目的：
  测试 AtomS3R + Atomic Voice Base 能不能直接播放后端生成的 TTS MP3。

  这个文件只测试“硬件说话”，不接 PaHUB、不接 OLED、不接传感器。
  如果这个测试能从 Voice Base 扬声器播放声音，再把同样逻辑合并进主程序。

  使用前请确认：
  1. 后端已经运行，并且 /audio/latest.mp3 能访问。
  2. 后端已经配置 DASHSCOPE_API_KEY，并且 /plant 调用后能生成 latest.mp3。
  3. Arduino IDE 已安装 ESP8266Audio 库。

  注意：
  - API Key 不要写进 Arduino。Arduino 只访问你们自己的 Flask 后端。
  - 当前使用 HTTP 地址。如果未来改 HTTPS，ESP8266Audio 的 HTTP stream 还要另行处理证书/安全客户端。
*/

#include <Arduino.h>
#include <WiFi.h>
#include <M5Unified.h>

#include <AudioOutput.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include "arduino_secrets.h"

const char* WIFI_SSID = SECRET_WIFI_SSID;
const char* WIFI_PASSWORD = SECRET_WIFI_PASSWORD;

// 后端生成的最新语音文件。
// 真实服务器地址写在本目录的 arduino_secrets.h 中。
const char* AUDIO_URL = SECRET_AUDIO_URL;

static constexpr uint8_t M5_SPK_CHANNEL = 0;
static constexpr int PREALLOCATE_BUFFER_SIZE = 5 * 1024;
static constexpr int PREALLOCATE_CODEC_SIZE = 29192;

class AudioOutputM5Speaker : public AudioOutput {
 public:
  AudioOutputM5Speaker(m5::Speaker_Class* speaker, uint8_t channel = 0) {
    speaker_ = speaker;
    channel_ = channel;
  }

  bool begin(void) override {
    return true;
  }

  bool ConsumeSample(int16_t sample[2]) override {
    if (bufferIndex_ < BUFFER_SIZE) {
      buffer_[bufferNumber_][bufferIndex_] = sample[0];
      buffer_[bufferNumber_][bufferIndex_ + 1] = sample[1];
      bufferIndex_ += 2;
      return true;
    }

    flush();
    return false;
  }

  void flush(void) override {
    if (bufferIndex_ == 0) return;

    speaker_->playRaw(
      buffer_[bufferNumber_],
      bufferIndex_,
      hertz,
      true,
      1,
      channel_
    );

    bufferNumber_ = bufferNumber_ < 2 ? bufferNumber_ + 1 : 0;
    bufferIndex_ = 0;
  }

  bool stop(void) override {
    flush();
    speaker_->stop(channel_);
    return true;
  }

 private:
  m5::Speaker_Class* speaker_ = nullptr;
  uint8_t channel_ = 0;
  static constexpr size_t BUFFER_SIZE = 640;
  int16_t buffer_[3][BUFFER_SIZE];
  size_t bufferIndex_ = 0;
  size_t bufferNumber_ = 0;
};

static AudioOutputM5Speaker audioOut(&M5.Speaker, M5_SPK_CHANNEL);
static AudioGeneratorMP3* mp3 = nullptr;
static AudioFileSourceHTTPStream* httpFile = nullptr;
static AudioFileSourceBuffer* bufferedFile = nullptr;
static void* preallocateBuffer = nullptr;
static void* preallocateCodec = nullptr;

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected.");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi failed.");
  }
}

void stopPlayback() {
  if (mp3 != nullptr) {
    mp3->stop();
    delete mp3;
    mp3 = nullptr;
  }

  if (bufferedFile != nullptr) {
    bufferedFile->close();
    delete bufferedFile;
    bufferedFile = nullptr;
  }

  if (httpFile != nullptr) {
    httpFile->close();
    delete httpFile;
    httpFile = nullptr;
  }

  M5.Speaker.stop();
}

bool startPlayback() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot play: WiFi is not connected.");
    return false;
  }

  stopPlayback();

  Serial.print("Opening MP3: ");
  Serial.println(AUDIO_URL);

  httpFile = new AudioFileSourceHTTPStream(AUDIO_URL);
  bufferedFile = new AudioFileSourceBuffer(httpFile, preallocateBuffer, PREALLOCATE_BUFFER_SIZE);
  mp3 = new AudioGeneratorMP3(preallocateCodec, PREALLOCATE_CODEC_SIZE);

  bool ok = mp3->begin(bufferedFile, &audioOut);
  Serial.println(ok ? "MP3 playback started." : "MP3 playback failed to start.");
  return ok;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  auto cfg = M5.config();

  // Atomic Voice Base 更接近 ATOMIC Speaker 这类外接扬声器。
  // 如果你们现场发现没有声音，可尝试把 atomic_spk 改成 atomic_echo。
  cfg.external_speaker.atomic_spk = true;
  cfg.external_speaker.atomic_echo = true;

  M5.begin(cfg);
  M5.Display.setBrightness(120);
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(4, 8);
  M5.Display.println("MP3 HTTP test");

  auto spkCfg = M5.Speaker.config();
  spkCfg.sample_rate = 64000;
  M5.Speaker.config(spkCfg);
  M5.Speaker.begin();
  M5.Speaker.setVolume(180);

  preallocateBuffer = malloc(PREALLOCATE_BUFFER_SIZE);
  preallocateCodec = malloc(PREALLOCATE_CODEC_SIZE);

  if (preallocateBuffer == nullptr || preallocateCodec == nullptr) {
    Serial.println("Failed to allocate MP3 buffers.");
    M5.Display.setCursor(4, 28);
    M5.Display.println("buffer failed");
    return;
  }

  connectWiFi();

  M5.Display.setCursor(4, 28);
  M5.Display.println("Press button");
  M5.Display.setCursor(4, 40);
  M5.Display.println("to play mp3");
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    startPlayback();
  }

  if (mp3 != nullptr) {
    if (mp3->isRunning()) {
      if (!mp3->loop()) {
        Serial.println("MP3 playback finished.");
        stopPlayback();
      }
    } else {
      stopPlayback();
    }
  }
}

