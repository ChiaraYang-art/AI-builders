/*
  City Sprout / 出走小芽
  实验版：DLight + IMU + Mic + Flask

  使用前请在本文件顶部修改 WIFI_SSID / WIFI_PASSWORD / SERVER_URL。
*/

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <M5Unified.h>
#include <math.h>

const char* WIFI_SSID = "DAElab";
const char* WIFI_PASSWORD = "tjdaelab";
const char* SERVER_URL = "http://111.229.81.45:5000/plant";

#define SDA_PIN 2
#define SCL_PIN 1
#define BH1750_ADDR 0x23

#define LUX_NEED_SUN_MAX 50
#define LUX_INDOOR_MAX 500
#define LUX_BRIGHT_MAX 800

const float OUTSIDE_LIGHT_LUX = 800.0;
const float ACTIVE_THRESHOLD = 0.06;
const unsigned long ACTIVE_HOLD_TIME_MS = 1500;

const int SOUND_SAMPLE_RATE = 16000;
const int SOUND_SAMPLE_COUNT = 256;
const int SOUND_HISTORY_SIZE = 12;
const unsigned long SOUND_INTERVAL_MS = 500;
const float SOUND_DYNAMIC_RANGE_THRESHOLD = 0.020;
const float SOUND_DYNAMIC_VARIANCE_THRESHOLD = 0.00008;

const unsigned long DRAW_INTERVAL_MS = 120;
const unsigned long SERVER_INTERVAL_MS = 8000;
const unsigned long PRINT_INTERVAL_MS = 800;

enum PlantState { STATE_IDLE, STATE_WILTED, STATE_NEED_SUN, STATE_SUNLIGHT, STATE_WALKING };
enum PlantMood { MOOD_NEED_SUN, MOOD_INDOOR, MOOD_BRIGHT, MOOD_SUNLIGHT };
enum MotionState { MOTION_STILL, MOTION_ACTIVE };
enum SoundState { SOUND_UNKNOWN, SOUND_STABLE, SOUND_DYNAMIC };
enum PlaceState { PLACE_UNKNOWN, PLACE_INDOOR, PLACE_OUTSIDE };

PlantMood currentMood = MOOD_INDOOR;
PlantState currentState = STATE_IDLE;
MotionState currentMotion = MOTION_STILL;
SoundState currentSound = SOUND_UNKNOWN;
PlaceState currentPlace = PLACE_UNKNOWN;

int16_t micBuffer[SOUND_SAMPLE_COUNT];
float soundHistory[SOUND_HISTORY_SIZE];
int soundHistoryIndex = 0;
int soundHistoryCount = 0;
bool micReady = false;

unsigned long lastActiveTime = 0;
unsigned long lastDrawTime = 0;
unsigned long lastServerTime = 0;
unsigned long lastPrintTime = 0;
unsigned long lastSoundTime = 0;

int frame = 0;
float lastLux = -1;
float lastMotionStrength = 0;
float lastSoundLevel = 0.0;
float lastSoundRange = 0.0;
float lastSoundVariance = 0.0;
String lastSpeech = "Waiting for light.";

uint16_t COLOR_BG;
uint16_t COLOR_STEM;
uint16_t COLOR_LEAF;
uint16_t COLOR_LEAF_LIGHT;
uint16_t COLOR_LEAF_DARK;
uint16_t COLOR_SUN;
uint16_t COLOR_TEXT;
uint16_t COLOR_POT;

String plantStateToText(PlantState state) {
  if (state == STATE_WILTED) return "wilted";
  if (state == STATE_NEED_SUN) return "need_sun";
  if (state == STATE_SUNLIGHT) return "sunlight";
  if (state == STATE_WALKING) return "walking";
  return "idle";
}

String motionToText(MotionState motion) {
  return motion == MOTION_ACTIVE ? "walking" : "still";
}

String soundToText(SoundState sound) {
  if (sound == SOUND_STABLE) return "stable";
  if (sound == SOUND_DYNAMIC) return "dynamic";
  return "unknown";
}

String placeToText(PlaceState place) {
  if (place == PLACE_INDOOR) return "indoor";
  if (place == PLACE_OUTSIDE) return "outside";
  return "unknown";
}

void writeBH1750Command(byte command) {
  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(command);
  Wire.endTransmission();
}

void initBH1750() {
  writeBH1750Command(0x01);
  delay(10);
  writeBH1750Command(0x10);
  delay(200);
}

float readLightLux() {
  Wire.requestFrom(BH1750_ADDR, 2);
  if (Wire.available() == 2) {
    byte highByte = Wire.read();
    byte lowByte = Wire.read();
    unsigned int rawValue = (highByte << 8) | lowByte;
    return rawValue / 1.2;
  }
  return -1;
}

PlantMood getMoodFromLux(float lux) {
  if (lux < LUX_NEED_SUN_MAX) return MOOD_NEED_SUN;
  if (lux < LUX_INDOOR_MAX) return MOOD_INDOOR;
  if (lux < LUX_BRIGHT_MAX) return MOOD_BRIGHT;
  return MOOD_SUNLIGHT;
}

PlantState getStateFromLux(float lux) {
  if (lux < LUX_NEED_SUN_MAX) return STATE_WILTED;
  if (lux < LUX_INDOOR_MAX) return STATE_IDLE;
  if (lux < LUX_BRIGHT_MAX) return STATE_NEED_SUN;
  return STATE_SUNLIGHT;
}

float getMotionStrength(float ax, float ay, float az) {
  float totalAcc = sqrt(ax * ax + ay * ay + az * az);
  return abs(totalAcc - 1.0);
}

MotionState updateMotionState(float motionStrength) {
  unsigned long now = millis();
  if (motionStrength > ACTIVE_THRESHOLD) {
    lastActiveTime = now;
    return MOTION_ACTIVE;
  }
  if (now - lastActiveTime < ACTIVE_HOLD_TIME_MS) return MOTION_ACTIVE;
  return MOTION_STILL;
}

void initMic() {
  /*
    Atomic Voice Base 上的麦克风不是 AtomS3R 主板自带的普通 ADC 麦克风。
    它挂在底座的音频硬件上，所以这里必须先把 M5.Mic 的参数配置好，
    再调用 M5.Mic.begin()。

    这些参数和 voice_base_mic_test.ino 里已经测试成功的参数保持一致：
    - sample_rate：每秒采样次数，这里 16000 表示 16kHz，足够判断环境声音变化。
    - dma_buf_count / dma_buf_len：底层录音缓冲区大小，太小容易录不到稳定数据。
    - noise_filter_level = 0：先关闭降噪，避免把环境声变化过滤掉。
    - over_sampling = 1：使用基础采样，不额外放大采样压力。
    - magnification：如果是 ADC 麦克风就放大 16 倍；I2S 麦克风一般保持 1 倍。
  */
  auto micCfg = M5.Mic.config();
  micCfg.sample_rate = SOUND_SAMPLE_RATE;
  micCfg.dma_buf_count = 3;
  micCfg.dma_buf_len = SOUND_SAMPLE_COUNT;
  micCfg.noise_filter_level = 0;
  micCfg.over_sampling = 1;
  micCfg.magnification = micCfg.use_adc ? 16 : 1;
  M5.Mic.config(micCfg);

  micReady = M5.Mic.begin();

  Serial.print("M5.Mic.begin(): ");
  Serial.println(micReady ? "true" : "false");
  Serial.print("M5.Mic.isEnabled(): ");
  Serial.println(M5.Mic.isEnabled() ? "true" : "false");
  Serial.println(micReady ? "Mic ready." : "Mic not ready. Sound detection will be unknown.");
}

float calculateSoundLevelFromBuffer() {
  double sumSquares = 0;
  for (int i = 0; i < SOUND_SAMPLE_COUNT; i++) {
    float sample = micBuffer[i] / 32768.0;
    sumSquares += sample * sample;
  }
  return sqrt(sumSquares / SOUND_SAMPLE_COUNT);
}

void pushSoundHistory(float level) {
  soundHistory[soundHistoryIndex] = level;
  soundHistoryIndex = (soundHistoryIndex + 1) % SOUND_HISTORY_SIZE;
  if (soundHistoryCount < SOUND_HISTORY_SIZE) soundHistoryCount++;
}

SoundState updateSoundState() {
  if (!micReady || !M5.Mic.isEnabled()) return SOUND_UNKNOWN;

  bool recorded = M5.Mic.record(micBuffer, SOUND_SAMPLE_COUNT, SOUND_SAMPLE_RATE);
  if (!recorded) return SOUND_UNKNOWN;

  lastSoundLevel = calculateSoundLevelFromBuffer();
  pushSoundHistory(lastSoundLevel);
  if (soundHistoryCount < 4) return SOUND_UNKNOWN;

  float minLevel = soundHistory[0];
  float maxLevel = soundHistory[0];
  float sum = 0;
  for (int i = 0; i < soundHistoryCount; i++) {
    float level = soundHistory[i];
    if (level < minLevel) minLevel = level;
    if (level > maxLevel) maxLevel = level;
    sum += level;
  }

  float mean = sum / soundHistoryCount;
  float varianceSum = 0;
  for (int i = 0; i < soundHistoryCount; i++) {
    float diff = soundHistory[i] - mean;
    varianceSum += diff * diff;
  }

  lastSoundRange = maxLevel - minLevel;
  lastSoundVariance = varianceSum / soundHistoryCount;

  if (lastSoundRange > SOUND_DYNAMIC_RANGE_THRESHOLD ||
      lastSoundVariance > SOUND_DYNAMIC_VARIANCE_THRESHOLD) {
    return SOUND_DYNAMIC;
  }
  return SOUND_STABLE;
}

PlaceState updatePlaceState(float lux, MotionState motion, SoundState sound) {
  int outsideScore = 0;
  if (lux >= OUTSIDE_LIGHT_LUX) outsideScore++;
  if (motion == MOTION_ACTIVE) outsideScore++;
  if (sound == SOUND_DYNAMIC) outsideScore++;

  if (outsideScore >= 2) return PLACE_OUTSIDE;
  if (lux < LUX_INDOOR_MAX && motion == MOTION_STILL && sound == SOUND_STABLE) return PLACE_INDOOR;
  return PLACE_UNKNOWN;
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected.");
    Serial.print("AtomS3R IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi failed. Local speech will be used.");
  }
}

String getLocalPlantSpeech(PlantState state) {
  if (state == STATE_WALKING) return "Are we going outside?";
  if (state == STATE_WILTED) return "I only saw screen light today.";
  if (state == STATE_NEED_SUN) return "Please take me to real sun.";
  if (state == STATE_SUNLIGHT) return "Sunlight found. I feel alive.";
  return "I am waiting for light.";
}

String askPlantServer(PlantState state, float lux, String motion, SoundState sound, PlaceState place) {
  if (WiFi.status() != WL_CONNECTED) return getLocalPlantSpeech(state);

  HTTPClient http;
  WiFiClient plainClient;
  WiFiClientSecure secureClient;

  if (String(SERVER_URL).startsWith("https://")) {
    // Demo 阶段先不校验证书，避免证书配置挡住课程演示。
    // 正式产品应改成 setCACert() 校验证书。
    secureClient.setInsecure();
    http.begin(secureClient, SERVER_URL);
  } else {
    http.begin(plainClient, SERVER_URL);
  }

  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"state\":\"" + plantStateToText(state) + "\",";
  payload += "\"lux\":" + String(lux, 2) + ",";
  payload += "\"motion\":\"" + motion + "\",";
  payload += "\"sound_level\":" + String(lastSoundLevel, 5) + ",";
  payload += "\"sound_range\":" + String(lastSoundRange, 5) + ",";
  payload += "\"sound_variance\":" + String(lastSoundVariance, 7) + ",";
  payload += "\"sound_state\":\"" + soundToText(sound) + "\",";
  payload += "\"place\":\"" + placeToText(place) + "\"";
  payload += "}";

  Serial.print("Send to server: ");
  Serial.println(payload);

  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    String response = http.getString();
    http.end();
    if (response.length() > 0) return response;
  } else {
    Serial.print("HTTP failed. Code: ");
    Serial.println(httpCode);
    http.end();
  }

  return getLocalPlantSpeech(state);
}

void initColors() {
  COLOR_BG = M5.Display.color565(12, 16, 14);
  COLOR_STEM = M5.Display.color565(210, 230, 200);
  COLOR_LEAF = M5.Display.color565(70, 160, 80);
  COLOR_LEAF_LIGHT = M5.Display.color565(110, 220, 95);
  COLOR_LEAF_DARK = M5.Display.color565(48, 85, 55);
  COLOR_SUN = M5.Display.color565(255, 205, 80);
  COLOR_TEXT = M5.Display.color565(230, 240, 220);
  COLOR_POT = M5.Display.color565(90, 60, 40);
}

void drawPlantOnM5(float lux, MotionState motion, SoundState sound, PlaceState place) {
  M5.Display.fillScreen(COLOR_BG);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
  M5.Display.setCursor(4, 4);
  M5.Display.print((int)lux);
  M5.Display.print("lx");
  M5.Display.setCursor(4, 16);
  M5.Display.print(motionToText(motion));
  M5.Display.setCursor(4, 28);
  M5.Display.print(soundToText(sound));
  M5.Display.setCursor(78, 4);
  M5.Display.print(placeToText(place));

  PlantMood mood = getMoodFromLux(lux);
  int phase = frame % 20;
  int sway = phase < 5 ? -2 : phase < 10 ? -1 : phase < 15 ? 1 : 2;
  int breathe = (frame % 30 < 15) ? 0 : 2;
  int centerX = 64;
  int baseY = 112;
  int topY = 44 + breathe;
  int leafY = 70;
  int leafDrop = 0;
  int plantSway = motion == MOTION_ACTIVE ? sway * 3 : sway;
  uint16_t leafColor = COLOR_LEAF;

  if (mood == MOOD_NEED_SUN) {
    topY = 62;
    leafY = 80;
    leafDrop = 12;
    plantSway = motion == MOTION_ACTIVE ? plantSway : 0;
    leafColor = COLOR_LEAF_DARK;
  } else if (mood == MOOD_BRIGHT) {
    topY = 38 + breathe;
    leafY = 66;
    leafDrop = -2;
    leafColor = COLOR_LEAF_LIGHT;
  } else if (mood == MOOD_SUNLIGHT) {
    topY = 32 + breathe;
    leafY = 62;
    leafDrop = -6;
    leafColor = COLOR_LEAF_LIGHT;
    M5.Display.fillCircle(104, 22, 7, COLOR_SUN);
  }

  if (sound == SOUND_DYNAMIC) {
    M5.Display.drawCircle(18, 42, 3, COLOR_TEXT);
    M5.Display.drawCircle(18, 42, 7, COLOR_TEXT);
  }

  M5.Display.drawLine(centerX - 2, baseY, centerX + plantSway, topY, COLOR_STEM);
  M5.Display.drawLine(centerX - 1, baseY, centerX + plantSway + 1, topY, COLOR_STEM);
  M5.Display.drawLine(centerX, baseY, centerX + plantSway + 2, topY, COLOR_STEM);
  M5.Display.fillEllipse(centerX - 26 + plantSway, leafY + leafDrop, 24, 11, leafColor);
  M5.Display.fillEllipse(centerX + 26 + plantSway, leafY + leafDrop + 4, 24, 11, leafColor);
  M5.Display.fillEllipse(centerX + plantSway + 8, topY + 4, mood == MOOD_SUNLIGHT ? 12 : 9, mood == MOOD_SUNLIGHT ? 6 : 5, leafColor);
  M5.Display.fillRoundRect(42, 112, 44, 10, 5, COLOR_POT);
  M5.Display.drawRoundRect(40, 110, 48, 14, 5, COLOR_TEXT);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  auto cfg = M5.config();
  /*
    Atomic Voice Base 属于 AtomS3R 下面接的音频底座。
    这里打开 atomic_echo 配置，是为了告诉 M5Unified：
    “现在外接的是带麦克风/扬声器的 Atomic Voice Base，请初始化它的音频硬件。”
    如果不写这一行，单独的麦克风测试可能能工作，但主程序里 M5.Mic.begin() 经常会失败。
  */
  cfg.external_speaker.atomic_echo = true;
  M5.begin(cfg);
  M5.Display.setBrightness(120);

  /*
    本程序现在只需要“录音检测环境声”，暂时不需要从底座扬声器播放声音。
    先关闭 Speaker，可以避免扬声器占用音频硬件资源，导致麦克风初始化失败。
    后续真的要 TTS 播放时，再单独处理 Mic 和 Speaker 的切换。
  */
  M5.Speaker.end();

  initColors();
  Wire.begin(SDA_PIN, SCL_PIN);
  initBH1750();
  initMic();
  connectWiFi();
}

void loop() {
  M5.update();
  unsigned long now = millis();

  if (now - lastSoundTime >= SOUND_INTERVAL_MS) {
    currentSound = updateSoundState();
    lastSoundTime = now;
  }

  if (now - lastDrawTime >= DRAW_INTERVAL_MS) {
    lastLux = readLightLux();
    if (lastLux < 0) {
      Serial.println("Failed to read DLight.");
      lastDrawTime = now;
      return;
    }

    float ax = 0;
    float ay = 0;
    float az = 0;
    M5.Imu.getAccelData(&ax, &ay, &az);
    lastMotionStrength = getMotionStrength(ax, ay, az);
    currentMotion = updateMotionState(lastMotionStrength);

    currentState = getStateFromLux(lastLux);
    if (currentMotion == MOTION_ACTIVE) currentState = STATE_WALKING;
    currentPlace = updatePlaceState(lastLux, currentMotion, currentSound);

    drawPlantOnM5(lastLux, currentMotion, currentSound, currentPlace);

    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
      Serial.print("Lux:");
      Serial.print(lastLux);
      Serial.print(" | motion:");
      Serial.print(motionToText(currentMotion));
      Serial.print(" strength:");
      Serial.print(lastMotionStrength, 3);
      Serial.print(" | sound:");
      Serial.print(soundToText(currentSound));
      Serial.print(" level:");
      Serial.print(lastSoundLevel, 5);
      Serial.print(" range:");
      Serial.print(lastSoundRange, 5);
      Serial.print(" var:");
      Serial.print(lastSoundVariance, 7);
      Serial.print(" | place:");
      Serial.println(placeToText(currentPlace));
      lastPrintTime = now;
    }

    frame++;
    lastDrawTime = now;
  }

  if (lastLux >= 0 && now - lastServerTime >= SERVER_INTERVAL_MS) {
    lastSpeech = askPlantServer(currentState, lastLux, motionToText(currentMotion), currentSound, currentPlace);
    Serial.print("Plant says: ");
    Serial.println(lastSpeech);
    lastServerTime = now;
  }
}
