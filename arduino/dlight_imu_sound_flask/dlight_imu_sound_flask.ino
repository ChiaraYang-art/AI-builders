/*
  City Sprout / 出走小芽
  文件：dlight_imu_sound_flask.ino

  这是从当前主程序复制出来的“声音实验版”。

  它比 dlight_flask_serial.ino 多了一个输入：
  - 麦克风声音环境变化检测

  当前功能：
  1. DLight 读取光照 lux。
  2. AtomS3R 内置 IMU 判断 still / walking。
  3. 麦克风读取环境声音大小，判断 stable / dynamic。
  4. 用“光照充足 / 正在移动 / 声音变化大”三个条件判断是否像是在出门。
  5. 把 state / lux / motion / sound / place 发给 Flask 后端。
  6. AtomS3R 彩屏继续显示彩色小芽动画。

  重要说明：
  - 这个版本先做“声音变化检测”，不是识别声音内容。
  - 也就是说，它能判断环境是否稳定、是否有明显波动；
    但还不能判断“听到了车、鸟、人声”。
  - 将来的录音 + 大模型识别，建议放到电脑 Flask 后端做。
*/

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <M5Unified.h>
#include <math.h>

// ============================================================
// 1. 现场需要修改的网络配置
// ============================================================

const char* WIFI_SSID = "DAElab";
const char* WIFI_PASSWORD = "tjdaelab";

// 这里必须使用电脑的局域网 IP，不能写 127.0.0.1。
const char* SERVER_URL = "http://192.168.10.36:5000/plant";

// ============================================================
// 2. DLight / BH1750 配置
// ============================================================

#define SDA_PIN 2
#define SCL_PIN 1
#define BH1750_ADDR 0x23

#define LUX_NEED_SUN_MAX 50
#define LUX_INDOOR_MAX 500
#define LUX_BRIGHT_MAX 800

// 认为“光照充足、可能在户外”的阈值。
const float OUTSIDE_LIGHT_LUX = 800.0;

// ============================================================
// 3. IMU 移动配置
// ============================================================

const float ACTIVE_THRESHOLD = 0.06;
const unsigned long ACTIVE_HOLD_TIME_MS = 1500;
unsigned long lastActiveTime = 0;

// ============================================================
// 4. 麦克风声音检测配置
// ============================================================

/*
  麦克风检测思路：
  - 每次录一小段声音。
  - 算出这一小段声音的 soundLevel。
  - 保存最近几次 soundLevel。
  - 如果最近声音大小变化范围很小，就是 stable。
  - 如果最近声音大小变化范围明显，就是 dynamic。

  阈值需要现场实测调整：
  - 如果教室里也经常 dynamic，把 SOUND_DYNAMIC_RANGE_THRESHOLD 调大。
  - 如果室外变化不明显，把它调小。
*/
const int SOUND_SAMPLE_RATE = 16000;
const int SOUND_SAMPLE_COUNT = 256;
const int SOUND_HISTORY_SIZE = 12;
const unsigned long SOUND_INTERVAL_MS = 500;
const float SOUND_DYNAMIC_RANGE_THRESHOLD = 0.020;
const float SOUND_DYNAMIC_VARIANCE_THRESHOLD = 0.00008;

int16_t micBuffer[SOUND_SAMPLE_COUNT];
float soundHistory[SOUND_HISTORY_SIZE];
int soundHistoryIndex = 0;
int soundHistoryCount = 0;

bool micReady = false;
unsigned long lastSoundTime = 0;

float lastSoundLevel = 0.0;
float lastSoundRange = 0.0;
float lastSoundVariance = 0.0;

// ============================================================
// 5. 状态定义
// ============================================================

enum PlantState {
  STATE_IDLE = 0,
  STATE_WILTED = 1,
  STATE_NEED_SUN = 2,
  STATE_SUNLIGHT = 3,
  STATE_WALKING = 4
};

enum PlantMood {
  MOOD_NEED_SUN = 0,
  MOOD_INDOOR = 1,
  MOOD_BRIGHT = 2,
  MOOD_SUNLIGHT = 3
};

enum MotionState {
  MOTION_STILL = 0,
  MOTION_ACTIVE = 1
};

enum SoundState {
  SOUND_UNKNOWN = 0,
  SOUND_STABLE = 1,
  SOUND_DYNAMIC = 2
};

enum PlaceState {
  PLACE_UNKNOWN = 0,
  PLACE_INDOOR = 1,
  PLACE_OUTSIDE = 2
};

PlantMood currentMood = MOOD_INDOOR;
PlantState currentState = STATE_IDLE;
MotionState currentMotion = MOTION_STILL;
SoundState currentSound = SOUND_UNKNOWN;
PlaceState currentPlace = PLACE_UNKNOWN;

// ============================================================
// 6. 时间和显示变量
// ============================================================

const unsigned long DRAW_INTERVAL_MS = 120;
const unsigned long SERVER_INTERVAL_MS = 8000;
const unsigned long PRINT_INTERVAL_MS = 800;

unsigned long lastDrawTime = 0;
unsigned long lastServerTime = 0;
unsigned long lastPrintTime = 0;

int frame = 0;

float lastLux = -1;
float lastMotionStrength = 0;
String lastSpeech = "Waiting for light.";

uint16_t COLOR_BG;
uint16_t COLOR_STEM;
uint16_t COLOR_LEAF;
uint16_t COLOR_LEAF_LIGHT;
uint16_t COLOR_LEAF_DARK;
uint16_t COLOR_SUN;
uint16_t COLOR_TEXT;
uint16_t COLOR_POT;

// ============================================================
// 7. 文本转换
// ============================================================

String plantStateToText(PlantState state) {
  if (state == STATE_IDLE) return "idle";
  if (state == STATE_WILTED) return "wilted";
  if (state == STATE_NEED_SUN) return "need_sun";
  if (state == STATE_SUNLIGHT) return "sunlight";
  if (state == STATE_WALKING) return "walking";
  return "idle";
}

String motionToText(MotionState motion) {
  if (motion == MOTION_ACTIVE) return "walking";
  return "still";
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

const char* moodToText(PlantMood mood) {
  if (mood == MOOD_NEED_SUN) return "NEED SUN";
  if (mood == MOOD_INDOOR) return "INDOOR";
  if (mood == MOOD_BRIGHT) return "BRIGHT";
  if (mood == MOOD_SUNLIGHT) return "SUNLIGHT";
  return "UNKNOWN";
}

// ============================================================
// 8. 光照函数
// ============================================================

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

// ============================================================
// 9. IMU 移动检测
// ============================================================

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

  if (now - lastActiveTime < ACTIVE_HOLD_TIME_MS) {
    return MOTION_ACTIVE;
  }

  return MOTION_STILL;
}

// ============================================================
// 10. 麦克风声音变化检测
// ============================================================

void initMic() {
  /*
    作用：
    启动 M5Unified 的麦克风接口。

    如果这里返回 false，说明当前板子/底座/库配置没有成功启用麦克风。
    程序仍然会继续跑，只是 sound_state 会是 unknown。
  */
  micReady = M5.Mic.begin();

  if (micReady) {
    Serial.println("Mic ready.");
  } else {
    Serial.println("Mic not ready. Sound detection will be unknown.");
  }
}

float calculateSoundLevelFromBuffer() {
  /*
    作用：
    把一小段录音转换成 0 到 1 左右的声音大小。

    这里使用 RMS，也就是均方根。
    你可以简单理解成：这一小段声音平均有多“响”。
  */
  double sumSquares = 0;

  for (int i = 0; i < SOUND_SAMPLE_COUNT; i++) {
    float sample = micBuffer[i] / 32768.0;
    sumSquares += sample * sample;
  }

  float rms = sqrt(sumSquares / SOUND_SAMPLE_COUNT);
  return rms;
}

void pushSoundHistory(float level) {
  soundHistory[soundHistoryIndex] = level;
  soundHistoryIndex = (soundHistoryIndex + 1) % SOUND_HISTORY_SIZE;

  if (soundHistoryCount < SOUND_HISTORY_SIZE) {
    soundHistoryCount++;
  }
}

SoundState updateSoundState() {
  /*
    作用：
    读取麦克风，并判断最近声音是 stable 还是 dynamic。

    stable：
    声音一直差不多，例如安静教室、固定空调噪声。

    dynamic：
    声音变化明显，例如走路时风声、车声、人声、环境切换。
  */
  if (!micReady) {
    currentSound = SOUND_UNKNOWN;
    return currentSound;
  }

  bool recorded = M5.Mic.record(micBuffer, SOUND_SAMPLE_COUNT, SOUND_SAMPLE_RATE);
  if (!recorded) {
    currentSound = SOUND_UNKNOWN;
    return currentSound;
  }

  lastSoundLevel = calculateSoundLevelFromBuffer();
  pushSoundHistory(lastSoundLevel);

  if (soundHistoryCount < 4) {
    currentSound = SOUND_UNKNOWN;
    return currentSound;
  }

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
    currentSound = SOUND_DYNAMIC;
  } else {
    currentSound = SOUND_STABLE;
  }

  return currentSound;
}

// ============================================================
// 11. 室内 / 出门判断
// ============================================================

PlaceState updatePlaceState(float lux, MotionState motion, SoundState sound) {
  /*
    作用：
    用三个条件判断现在更像 indoor 还是 outside。

    三个 outside 条件：
    1. 光照充足。
    2. 小芽正在移动。
    3. 声音变化大。

    满足其中两个，就判定为 outside。

    室内条件：
    1. 光照不足。
    2. 小芽 still。
    3. 声音 stable。
  */
  int outsideScore = 0;

  if (lux >= OUTSIDE_LIGHT_LUX) outsideScore++;
  if (motion == MOTION_ACTIVE) outsideScore++;
  if (sound == SOUND_DYNAMIC) outsideScore++;

  if (outsideScore >= 2) {
    return PLACE_OUTSIDE;
  }

  if (lux < LUX_INDOOR_MAX &&
      motion == MOTION_STILL &&
      sound == SOUND_STABLE) {
    return PLACE_INDOOR;
  }

  return PLACE_UNKNOWN;
}

// ============================================================
// 12. Wi-Fi 和 Flask
// ============================================================

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

String askPlantServer(
  PlantState state,
  float lux,
  String motion,
  SoundState sound,
  PlaceState place
) {
  if (WiFi.status() != WL_CONNECTED) {
    return getLocalPlantSpeech(state);
  }

  HTTPClient http;
  http.begin(SERVER_URL);
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

    if (response.length() > 0) {
      return response;
    }
  } else {
    Serial.print("HTTP failed. Code: ");
    Serial.println(httpCode);
    http.end();
  }

  return getLocalPlantSpeech(state);
}

// ============================================================
// 13. 彩屏小芽
// ============================================================

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

void drawPlantOnM5(PlantMood mood, MotionState motion, SoundState sound, PlaceState place, float lux) {
  M5.Display.fillScreen(COLOR_BG);

  int phase = frame % 20;
  int sway = 0;

  if (phase < 5) sway = -2;
  else if (phase < 10) sway = -1;
  else if (phase < 15) sway = 1;
  else sway = 2;

  int breathe = (frame % 30 < 15) ? 0 : 2;

  int centerX = 64;
  int baseY = 112;
  int topY = 42;
  int leafY = 68;
  int leafDrop = 0;
  int plantSway = sway;
  uint16_t leafColor = COLOR_LEAF;

  if (mood == MOOD_NEED_SUN) {
    topY = 62;
    leafY = 80;
    leafDrop = 12;
    leafColor = COLOR_LEAF_DARK;
  } else if (mood == MOOD_INDOOR) {
    topY = 44 + breathe;
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
  }

  if (motion == MOTION_ACTIVE) {
    plantSway = sway * 3;
  }

  if (mood == MOOD_NEED_SUN && motion == MOTION_STILL) {
    plantSway = 0;
  }

  if (mood == MOOD_SUNLIGHT) {
    M5.Display.fillCircle(104, 22, 7, COLOR_SUN);
    M5.Display.fillCircle(92, 36, 2, COLOR_SUN);
    M5.Display.fillCircle(113, 42, 2, COLOR_SUN);
  }

  if (motion == MOTION_ACTIVE) {
    M5.Display.drawLine(8, 54, 22, 54, COLOR_TEXT);
    M5.Display.drawLine(104, 72, 122, 72, COLOR_TEXT);
  }

  if (sound == SOUND_DYNAMIC) {
    M5.Display.drawCircle(18, 32, 3, COLOR_TEXT);
    M5.Display.drawCircle(18, 32, 7, COLOR_TEXT);
  }

  M5.Display.drawLine(centerX - 2, baseY, centerX + plantSway, topY, COLOR_STEM);
  M5.Display.drawLine(centerX - 1, baseY, centerX + plantSway + 1, topY, COLOR_STEM);
  M5.Display.drawLine(centerX, baseY, centerX + plantSway + 2, topY, COLOR_STEM);

  int leftLeafX = centerX - 26 + plantSway;
  int leftLeafY = leafY + leafDrop;
  int rightLeafX = centerX + 26 + plantSway;
  int rightLeafY = leafY + leafDrop + 4;

  M5.Display.fillEllipse(leftLeafX, leftLeafY, 24, 11, leafColor);
  M5.Display.fillEllipse(rightLeafX, rightLeafY, 24, 11, leafColor);

  int budX = centerX + plantSway + 8;
  int budY = topY + 4;

  if (mood == MOOD_NEED_SUN) {
    M5.Display.fillCircle(budX, budY + 4, 5, leafColor);
  } else if (mood == MOOD_SUNLIGHT) {
    M5.Display.fillEllipse(budX, budY, 12, 6, leafColor);
    M5.Display.drawLine(centerX + plantSway, topY + 8, budX, budY, COLOR_STEM);
  } else {
    M5.Display.fillEllipse(budX, budY, 9, 5, leafColor);
    M5.Display.drawLine(centerX + plantSway, topY + 8, budX, budY, COLOR_STEM);
  }

  M5.Display.fillRoundRect(42, 112, 44, 10, 5, COLOR_POT);
  M5.Display.drawRoundRect(40, 110, 48, 14, 5, COLOR_TEXT);

  // 左上角保留很小的调试信息，方便调阈值。
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
}

// ============================================================
// 14. Arduino 标准入口
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(120);

  initColors();

  Wire.begin(SDA_PIN, SCL_PIN);
  initBH1750();
  initMic();

  M5.Display.fillScreen(COLOR_BG);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(8, 8);
  M5.Display.print("Sprout sound demo");
  M5.Display.setCursor(8, 24);
  M5.Display.print("DLight IMU Mic");

  connectWiFi();

  Serial.println();
  Serial.println("DLight + IMU + Mic + Flask demo started.");
  Serial.println("Sound: stable / dynamic / unknown");
  Serial.println("Place: indoor / outside / unknown");
  Serial.println("--------------------------------");
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
      M5.Display.fillScreen(COLOR_BG);
      M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
      M5.Display.setTextSize(1);
      M5.Display.setCursor(8, 8);
      M5.Display.print("DLight read fail");
      Serial.println("Failed to read DLight.");
      lastDrawTime = now;
      return;
    }

    currentMood = getMoodFromLux(lastLux);

    float ax = 0;
    float ay = 0;
    float az = 0;
    M5.Imu.getAccelData(&ax, &ay, &az);

    lastMotionStrength = getMotionStrength(ax, ay, az);
    currentMotion = updateMotionState(lastMotionStrength);

    currentState = getStateFromLux(lastLux);
    if (currentMotion == MOTION_ACTIVE) {
      currentState = STATE_WALKING;
    }

    currentPlace = updatePlaceState(lastLux, currentMotion, currentSound);

    drawPlantOnM5(currentMood, currentMotion, currentSound, currentPlace, lastLux);

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
      Serial.println("--------------------------------");

      lastPrintTime = now;
    }

    frame++;
    lastDrawTime = now;
  }

  if (lastLux >= 0 && now - lastServerTime >= SERVER_INTERVAL_MS) {
    lastSpeech = askPlantServer(
      currentState,
      lastLux,
      motionToText(currentMotion),
      currentSound,
      currentPlace
    );

    Serial.print("Plant says: ");
    Serial.println(lastSpeech);
    Serial.println("--------------------------------");

    lastServerTime = now;
  }
}
