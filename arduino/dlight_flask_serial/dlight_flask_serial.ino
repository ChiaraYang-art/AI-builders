/*
  City Sprout / 出走小芽
  主程序：DLight + IMU + Flask

  使用前请在本文件顶部修改：
  - WIFI_SSID
  - WIFI_PASSWORD
  - SERVER_URL
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

const float ACTIVE_THRESHOLD = 0.06;
const unsigned long ACTIVE_HOLD_TIME_MS = 1500;
const unsigned long DRAW_INTERVAL_MS = 120;
const unsigned long SERVER_INTERVAL_MS = 8000;
const unsigned long PRINT_INTERVAL_MS = 800;

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

PlantMood currentMood = MOOD_INDOOR;
PlantState currentState = STATE_IDLE;
MotionState currentMotion = MOTION_STILL;

unsigned long lastActiveTime = 0;
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

const char* moodToText(PlantMood mood) {
  if (mood == MOOD_NEED_SUN) return "NEED SUN";
  if (mood == MOOD_INDOOR) return "INDOOR";
  if (mood == MOOD_BRIGHT) return "BRIGHT";
  if (mood == MOOD_SUNLIGHT) return "SUNLIGHT";
  return "UNKNOWN";
}

String getLocalPlantSpeech(PlantState state) {
  if (state == STATE_WALKING) return "Are we going outside?";
  if (state == STATE_WILTED) return "I only saw screen light today.";
  if (state == STATE_NEED_SUN) return "Please take me to real sun.";
  if (state == STATE_SUNLIGHT) return "Sunlight found. I feel alive.";
  return "I am waiting for light.";
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

  if (now - lastActiveTime < ACTIVE_HOLD_TIME_MS) {
    return MOTION_ACTIVE;
  }

  return MOTION_STILL;
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

String askPlantServer(PlantState state, float lux, String motion) {
  if (WiFi.status() != WL_CONNECTED) {
    return getLocalPlantSpeech(state);
  }

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
  payload += "\"motion\":\"" + motion + "\"";
  payload += "}";

  Serial.print("Send to server: ");
  Serial.println(payload);

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    String response = http.getString();
    http.end();

    Serial.print("Server response: ");
    Serial.println(response);

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

void drawPlantOnM5(PlantMood mood, MotionState motion, float lux) {
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

  if (motion == MOTION_ACTIVE) plantSway = sway * 3;
  if (mood == MOOD_NEED_SUN && motion == MOTION_STILL) plantSway = 0;

  if (mood == MOOD_SUNLIGHT) {
    M5.Display.fillCircle(104, 22, 7, COLOR_SUN);
    M5.Display.fillCircle(92, 36, 2, COLOR_SUN);
    M5.Display.fillCircle(113, 42, 2, COLOR_SUN);
  }

  if (motion == MOTION_ACTIVE) {
    M5.Display.drawLine(8, 54, 22, 54, COLOR_TEXT);
    M5.Display.drawLine(104, 72, 122, 72, COLOR_TEXT);
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
  } else {
    M5.Display.fillEllipse(budX, budY, mood == MOOD_SUNLIGHT ? 12 : 9, mood == MOOD_SUNLIGHT ? 6 : 5, leafColor);
    M5.Display.drawLine(centerX + plantSway, topY + 8, budX, budY, COLOR_STEM);
  }

  M5.Display.fillRoundRect(42, 112, 44, 10, 5, COLOR_POT);
  M5.Display.drawRoundRect(40, 110, 48, 14, 5, COLOR_TEXT);

  M5.Display.setTextSize(1);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
  M5.Display.setCursor(4, 4);
  M5.Display.print((int)lux);
  M5.Display.print("lx");
  M5.Display.setCursor(4, 16);
  M5.Display.print(motionToText(motion));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(120);

  initColors();
  Wire.begin(SDA_PIN, SCL_PIN);
  initBH1750();

  M5.Display.fillScreen(COLOR_BG);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(8, 8);
  M5.Display.print("Sprout demo");
  M5.Display.setCursor(8, 24);
  M5.Display.print("DLight + IMU");

  connectWiFi();
}

void loop() {
  M5.update();
  unsigned long now = millis();

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
    if (currentMotion == MOTION_ACTIVE) currentState = STATE_WALKING;

    drawPlantOnM5(currentMood, currentMotion, lastLux);

    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
      Serial.print("Lux:");
      Serial.print(lastLux);
      Serial.print(" | motion:");
      Serial.print(motionToText(currentMotion));
      Serial.print(" | strength:");
      Serial.print(lastMotionStrength, 3);
      Serial.print(" | state:");
      Serial.println(plantStateToText(currentState));
      lastPrintTime = now;
    }

    frame++;
    lastDrawTime = now;
  }

  if (lastLux >= 0 && now - lastServerTime >= SERVER_INTERVAL_MS) {
    lastSpeech = askPlantServer(currentState, lastLux, motionToText(currentMotion));
    Serial.print("Plant says: ");
    Serial.println(lastSpeech);
    lastServerTime = now;
  }
}
