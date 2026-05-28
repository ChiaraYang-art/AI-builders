/*
  City Sprout / 出走小芽
  文件：dlight_flask_serial.ino

  这是当前最重要的“传感器 + 动画 + 后端”主程序。

  当前功能：
  1. 读取 DLight 光照传感器，得到 lux 光照值。
  2. 使用 AtomS3R 内置 IMU，判断设备是静止还是正在被拿起/移动。
  3. 根据 lux 和 motion 改变 AtomS3R 自带彩屏上的彩色小芽动画。
  4. 每隔几秒把 state / lux / motion 发给电脑 Flask 后端。
  5. 接收 Flask 返回的一句植物语言，并在 Serial Monitor 打印。

  当前硬件连接：
  AtomS3R Grove / HY2.0-4P
  -> M5Stack DLight Unit

  当前不接 OLED。
  等 Unit Hub / PaHUB 到货后，再把 OLED 文字显示合并回来。
*/

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <M5Unified.h>
#include <math.h>
#include "arduino_secrets.h"

// ============================================================
// 1. 需要按现场网络修改的配置
// ============================================================

// Wi-Fi 名称。真实值写在本文件同目录的 arduino_secrets.h 中。
const char* WIFI_SSID = SECRET_WIFI_SSID;

// Wi-Fi 密码。真实值写在 arduino_secrets.h 中，不上传 GitHub。
const char* WIFI_PASSWORD = SECRET_WIFI_PASSWORD;

// Flask 后端地址。
// 非常重要：
// 这里不能写 127.0.0.1。
// 127.0.0.1 对 AtomS3R 来说是“AtomS3R 自己”，不是你的电脑。
//
// 正确写法示例：
// const char* SERVER_URL = "http://192.168.xx.xx:5000/plant";
//
// 这个地址写在 arduino_secrets.h 中，可以是局域网电脑，也可以是云服务器。
const char* SERVER_URL = SECRET_SERVER_URL;

// ============================================================
// 2. DLight / BH1750 设置
// ============================================================

// AtomS3R Grove 口的 I2C 数据线。
// 之前 OLED 和 DLight 单独测试都已经验证过：SDA 用 G2。
#define SDA_PIN 2

// AtomS3R Grove 口的 I2C 时钟线。
// 之前 OLED 和 DLight 单独测试都已经验证过：SCL 用 G1。
#define SCL_PIN 1

// DLight 内部使用 BH1750 光照芯片。
// 常见地址是 0x23。如果现场读不到，可以尝试改成 0x5C。
#define BH1750_ADDR 0x23

// ============================================================
// 3. 光照阈值
// ============================================================

/*
  这组阈值来自组员的实测结果：

  手捂住：约 0 lux
  桌面平放：约 447 lux
  手电筒 20cm：约 530 lux
  手电筒 10cm：约 915 lux

  所以先按 4 档处理：
  0-50 lux      NEED SUN  暗光，小芽低头
  50-500 lux    INDOOR    室内普通光，小芽轻微呼吸
  500-800 lux   BRIGHT    明亮，小芽更精神
  800+ lux      SUNLIGHT  强光，小芽展开叶子
*/
#define LUX_NEED_SUN_MAX 50
#define LUX_INDOOR_MAX 500
#define LUX_BRIGHT_MAX 800

// ============================================================
// 4. IMU 移动阈值
// ============================================================

/*
  AtomS3R 内置 IMU 会读到三轴加速度 ax / ay / az。

  静止放在桌上时：
  sqrt(ax^2 + ay^2 + az^2) 通常接近 1。

  被拿起、晃动、走路时：
  这个总加速度会偏离 1。

  ACTIVE_THRESHOLD 越小，越容易判定为“正在移动”。
  如果放桌上也经常误判移动，可以改大一点，例如 0.08。
  如果拿起来不够灵敏，可以改小一点，例如 0.04。
*/
const float ACTIVE_THRESHOLD = 0.06;

// 检测到移动后，保持 walking 状态 1.5 秒。
// 这样小芽不会因为瞬间停止就马上跳回 still，视觉更稳定。
const unsigned long ACTIVE_HOLD_TIME_MS = 1500;

unsigned long lastActiveTime = 0;

// ============================================================
// 5. 状态定义
// ============================================================

// PlantState 是给 Flask 后端使用的统一项目状态。
// 后端目前认识 idle / wilted / need_sun / sunlight / walking。
enum PlantState {
  STATE_IDLE = 0,
  STATE_WILTED = 1,
  STATE_NEED_SUN = 2,
  STATE_SUNLIGHT = 3,
  STATE_WALKING = 4
};

// PlantMood 是给 AtomS3R 彩屏动画使用的视觉状态。
// 它比后端状态多了 INDOOR 和 BRIGHT 两档，能更细腻地表现不同 lux。
enum PlantMood {
  MOOD_NEED_SUN = 0,
  MOOD_INDOOR = 1,
  MOOD_BRIGHT = 2,
  MOOD_SUNLIGHT = 3
};

// MotionState 是 IMU 的简化结果。
// 现在只分 still 和 active，后续可以继续细分 picked_up / walking。
enum MotionState {
  MOTION_STILL = 0,
  MOTION_ACTIVE = 1
};

PlantMood currentMood = MOOD_INDOOR;
PlantState currentState = STATE_IDLE;
MotionState currentMotion = MOTION_STILL;

// ============================================================
// 6. 时间控制
// ============================================================

// 彩屏动画刷新间隔。
// 120ms 比较接近 IMU 测试版，移动时小芽摆动更顺。
const unsigned long DRAW_INTERVAL_MS = 120;

// 请求 Flask 的间隔。
// 不要每一帧都请求后端，否则电脑终端会刷屏，Wi-Fi 也会更不稳定。
const unsigned long SERVER_INTERVAL_MS = 8000;

// Serial Monitor 打印间隔。
// IMU 数据很多，限制频率可以避免串口刷屏。
const unsigned long PRINT_INTERVAL_MS = 800;

unsigned long lastDrawTime = 0;
unsigned long lastServerTime = 0;
unsigned long lastPrintTime = 0;

// frame 用来制造小芽左右摆动、上下呼吸的动画。
int frame = 0;

// 记录最近一次读到的数据和后端返回文案。
float lastLux = -1;
float lastMotionStrength = 0;
String lastSpeech = "Waiting for light.";

// ============================================================
// 7. 彩屏颜色变量
// ============================================================

uint16_t COLOR_BG;
uint16_t COLOR_STEM;
uint16_t COLOR_LEAF;
uint16_t COLOR_LEAF_LIGHT;
uint16_t COLOR_LEAF_DARK;
uint16_t COLOR_SUN;
uint16_t COLOR_TEXT;
uint16_t COLOR_POT;

// ============================================================
// 8. 光照状态转换
// ============================================================

PlantMood getMoodFromLux(float lux) {
  /*
    作用：
    把 lux 转换成“彩屏动画状态”。

    返回值会决定小芽在 AtomS3R 彩屏上的形态。
  */
  if (lux < LUX_NEED_SUN_MAX) {
    return MOOD_NEED_SUN;
  }

  if (lux < LUX_INDOOR_MAX) {
    return MOOD_INDOOR;
  }

  if (lux < LUX_BRIGHT_MAX) {
    return MOOD_BRIGHT;
  }

  return MOOD_SUNLIGHT;
}

PlantState getStateFromLux(float lux) {
  /*
    作用：
    把 lux 转换成“后端语言状态”。

    注意：
    如果 IMU 检测到移动，loop() 里会把状态覆盖成 STATE_WALKING。
  */
  if (lux < LUX_NEED_SUN_MAX) {
    return STATE_WILTED;
  }

  if (lux < LUX_INDOOR_MAX) {
    return STATE_IDLE;
  }

  if (lux < LUX_BRIGHT_MAX) {
    return STATE_NEED_SUN;
  }

  return STATE_SUNLIGHT;
}

String plantStateToText(PlantState state) {
  if (state == STATE_IDLE) {
    return "idle";
  }

  if (state == STATE_WILTED) {
    return "wilted";
  }

  if (state == STATE_NEED_SUN) {
    return "need_sun";
  }

  if (state == STATE_SUNLIGHT) {
    return "sunlight";
  }

  if (state == STATE_WALKING) {
    return "walking";
  }

  return "idle";
}

const char* moodToText(PlantMood mood) {
  if (mood == MOOD_NEED_SUN) {
    return "NEED SUN";
  }

  if (mood == MOOD_INDOOR) {
    return "INDOOR";
  }

  if (mood == MOOD_BRIGHT) {
    return "BRIGHT";
  }

  if (mood == MOOD_SUNLIGHT) {
    return "SUNLIGHT";
  }

  return "UNKNOWN";
}

String motionToText(MotionState motion) {
  if (motion == MOTION_ACTIVE) {
    return "walking";
  }

  return "still";
}

String getLocalPlantSpeech(PlantState state) {
  /*
    作用：
    后端没开、Wi-Fi 没连上、HTTP 请求失败时，用本地固定文案兜底。
  */
  if (state == STATE_IDLE) {
    return "I am waiting for light.";
  }

  if (state == STATE_WILTED) {
    return "I only saw screen light today.";
  }

  if (state == STATE_NEED_SUN) {
    return "Please take me to real sun.";
  }

  if (state == STATE_SUNLIGHT) {
    return "Sunlight found. I feel alive.";
  }

  if (state == STATE_WALKING) {
    return "Are we going outside?";
  }

  return "I am here.";
}

// ============================================================
// 9. DLight / BH1750 函数
// ============================================================

void writeBH1750Command(byte command) {
  /*
    作用：
    向 BH1750 发送控制命令。

    0x01 = Power On，让芯片开机。
    0x10 = Continuous High Resolution Mode，让芯片连续读取光照。
  */
  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(command);
  Wire.endTransmission();
}

void initBH1750() {
  /*
    作用：
    初始化 DLight 光照传感器。
  */
  writeBH1750Command(0x01);
  delay(10);

  writeBH1750Command(0x10);
  delay(200);
}

float readLightLux() {
  /*
    作用：
    从 BH1750 读取 2 个字节，并换算成 lux。

    返回：
    - 正数：读取成功。
    - -1：读取失败，可能是线没接好、地址不对、或传感器没供电。
  */
  Wire.requestFrom(BH1750_ADDR, 2);

  if (Wire.available() == 2) {
    byte highByte = Wire.read();
    byte lowByte = Wire.read();

    unsigned int rawValue = (highByte << 8) | lowByte;
    float lux = rawValue / 1.2;

    return lux;
  }

  return -1;
}

// ============================================================
// 10. IMU 移动检测函数
// ============================================================

float getMotionStrength(float ax, float ay, float az) {
  /*
    作用：
    根据三轴加速度算出“移动强度”。

    静止时：
    sqrt(ax^2 + ay^2 + az^2) 接近 1。

    移动时：
    总加速度会偏离 1。

    返回值越大，说明移动越明显。
  */
  float totalAcc = sqrt(ax * ax + ay * ay + az * az);
  float motionStrength = abs(totalAcc - 1.0);

  return motionStrength;
}

MotionState updateMotionState(float motionStrength) {
  /*
    作用：
    把移动强度转换成 still / active。

    稳定策略：
    1. 当前移动强度超过阈值 -> ACTIVE。
    2. 最近 1.5 秒内检测到过 ACTIVE -> 继续保持 ACTIVE。
    3. 否则 -> STILL。
  */
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
// 11. Wi-Fi 和 Flask 请求
// ============================================================

void connectWiFi() {
  /*
    作用：
    连接 Wi-Fi。

    这里最多等 15 秒。
    如果连不上，程序仍然会继续显示小芽，只是不请求 Flask。
  */
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
  /*
    作用：
    把当前植物状态发给 Flask 后端，并读取后端返回的一句植物语言。
  */
  if (WiFi.status() != WL_CONNECTED) {
    return getLocalPlantSpeech(state);
  }

  HTTPClient http;
  http.begin(SERVER_URL);
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
// 12. AtomS3R 彩屏小芽
// ============================================================

void drawPlantOnM5(PlantMood mood, MotionState motion, float lux) {
  /*
    作用：
    在 AtomS3R 自带彩屏上画彩色小芽。

    光照影响：
    - 暗光：小芽低头、叶子暗。
    - 室内：正常呼吸。
    - 明亮：叶子更亮。
    - 强光：顶部更高，并出现太阳粒子。

    移动影响：
    - still：轻微呼吸。
    - walking：左右摆动更明显，并出现动线。
  */
  M5.Display.fillScreen(COLOR_BG);

  int phase = frame % 20;
  int sway = 0;

  if (phase < 5) {
    sway = -2;
  } else if (phase < 10) {
    sway = -1;
  } else if (phase < 15) {
    sway = 1;
  } else {
    sway = 2;
  }

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
    leafY = 70;
    leafDrop = 0;
    leafColor = COLOR_LEAF;
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
    M5.Display.fillCircle(88, 20, 1, COLOR_SUN);
  }

  if (motion == MOTION_ACTIVE) {
    M5.Display.drawLine(8, 54, 22, 54, COLOR_TEXT);
    M5.Display.drawLine(104, 72, 122, 72, COLOR_TEXT);
  }

  if (mood == MOOD_NEED_SUN) {
    M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(50, 18);
    M5.Display.print("...");
  }

  M5.Display.drawLine(centerX - 2, baseY, centerX + plantSway, topY, COLOR_STEM);
  M5.Display.drawLine(centerX - 1, baseY, centerX + plantSway + 1, topY, COLOR_STEM);
  M5.Display.drawLine(centerX, baseY, centerX + plantSway + 2, topY, COLOR_STEM);

  int leftLeafX = centerX - 26 + plantSway;
  int leftLeafY = leafY + leafDrop;

  M5.Display.fillEllipse(leftLeafX, leftLeafY, 24, 11, leafColor);
  M5.Display.drawLine(centerX + plantSway, leafY, leftLeafX - 18, leftLeafY, COLOR_STEM);
  M5.Display.drawLine(leftLeafX - 4, leftLeafY, leftLeafX - 12, leftLeafY - 5, COLOR_STEM);
  M5.Display.drawLine(leftLeafX - 4, leftLeafY, leftLeafX - 12, leftLeafY + 5, COLOR_STEM);

  int rightLeafX = centerX + 26 + plantSway;
  int rightLeafY = leafY + leafDrop + 4;

  M5.Display.fillEllipse(rightLeafX, rightLeafY, 24, 11, leafColor);
  M5.Display.drawLine(centerX + plantSway, leafY + 4, rightLeafX + 18, rightLeafY, COLOR_STEM);
  M5.Display.drawLine(rightLeafX + 4, rightLeafY, rightLeafX + 12, rightLeafY - 5, COLOR_STEM);
  M5.Display.drawLine(rightLeafX + 4, rightLeafY, rightLeafX + 12, rightLeafY + 5, COLOR_STEM);

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

  M5.Display.setTextSize(1);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);

  M5.Display.setCursor(4, 4);
  M5.Display.print((int)lux);
  M5.Display.print("lx");

  M5.Display.setCursor(4, 16);
  M5.Display.print(motionToText(motion));

  // 右下角显示 Flask 是否有返回过文案。
  M5.Display.setCursor(88, 116);
  if (lastSpeech.length() > 0) {
    M5.Display.print("AI");
  }
}

void initColors() {
  /*
    作用：
    初始化彩屏会用到的颜色。
  */
  COLOR_BG = M5.Display.color565(12, 16, 14);
  COLOR_STEM = M5.Display.color565(210, 230, 200);
  COLOR_LEAF = M5.Display.color565(70, 160, 80);
  COLOR_LEAF_LIGHT = M5.Display.color565(110, 220, 95);
  COLOR_LEAF_DARK = M5.Display.color565(48, 85, 55);
  COLOR_SUN = M5.Display.color565(255, 205, 80);
  COLOR_TEXT = M5.Display.color565(230, 240, 220);
  COLOR_POT = M5.Display.color565(90, 60, 40);
}

// ============================================================
// 13. Arduino 标准入口
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

  M5.Display.fillScreen(COLOR_BG);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(8, 8);
  M5.Display.print("Sprout demo");
  M5.Display.setCursor(8, 24);
  M5.Display.print("DLight + IMU");

  connectWiFi();

  Serial.println();
  Serial.println("DLight + IMU + Flask + M5 plant demo started.");
  Serial.println("--------------------------------");
  Serial.println("Light thresholds:");
  Serial.println("0-50    NEED SUN / WILTED");
  Serial.println("50-500  INDOOR / IDLE");
  Serial.println("500-800 BRIGHT / NEED_SUN");
  Serial.println("800+    SUNLIGHT / SUNLIGHT");
  Serial.println("--------------------------------");
  Serial.println("Motion:");
  Serial.println("still / walking");
  Serial.println("--------------------------------");
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
    if (currentMotion == MOTION_ACTIVE) {
      currentState = STATE_WALKING;
    }

    drawPlantOnM5(currentMood, currentMotion, lastLux);

    if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
      Serial.print("Lux: ");
      Serial.print(lastLux);
      Serial.print(" lx | Mood: ");
      Serial.print(moodToText(currentMood));
      Serial.print(" | Motion: ");
      Serial.print(motionToText(currentMotion));
      Serial.print(" | Strength: ");
      Serial.print(lastMotionStrength, 3);
      Serial.print(" | State: ");
      Serial.println(plantStateToText(currentState));
      Serial.println("--------------------------------");

      lastPrintTime = now;
    }

    frame++;
    lastDrawTime = now;
  }

  if (lastLux >= 0 && now - lastServerTime >= SERVER_INTERVAL_MS) {
    String motion = motionToText(currentMotion);
    lastSpeech = askPlantServer(currentState, lastLux, motion);

    Serial.print("Plant says: ");
    Serial.println(lastSpeech);
    Serial.println("--------------------------------");

    lastServerTime = now;
  }
}
