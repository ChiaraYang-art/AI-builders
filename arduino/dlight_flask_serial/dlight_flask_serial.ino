/*
  City Sprout / 出走小芽
  文件：dlight_flask_serial.ino

  这个程序对应“传感器 + 后端文案模式”：
  1. 只连接 DLight 光照传感器，不连接外接 OLED。
  2. AtomS3R 读取 lux 光照值。
  3. 根据 lux 判断植物状态。
  4. 把 state / lux / motion 通过 HTTP POST 发给电脑上的 Flask 后端。
  5. 接收后端返回的植物语言。
  6. 在 Serial Monitor 打印结果，也在 AtomS3R 自带彩屏上简单显示。

  当前硬件连接：
  AtomS3R Grove / HY2.0-4P
  -> M5Stack DLight Unit

  重要提醒：
  - AtomS3R 访问电脑后端时，不能使用 127.0.0.1。
  - SERVER_URL 必须改成电脑在同一 Wi-Fi 下的局域网 IP。
  - 例如电脑 Flask 显示 Running on http://192.168.10.36:5000
    那么这里就写 http://192.168.10.36:5000/plant
*/

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "M5AtomS3.h"

// =========================
// 1. 需要按现场网络修改的配置
// =========================

// Wi-Fi 名称。这里先保留实验室示例值，换教室或热点时需要修改。
const char* WIFI_SSID = "DAElab";

// Wi-Fi 密码。上传到公共仓库前请不要保留真实密码。
const char* WIFI_PASSWORD = "tjdaelab";

// Flask 后端地址。
// 注意：这里不能写 127.0.0.1，因为 127.0.0.1 对 AtomS3R 来说是它自己，不是电脑。
const char* SERVER_URL = "http://192.168.10.36:5000/plant";

// =========================
// 2. Grove / I2C 引脚设置
// =========================

// DLight 和 OLED 一样都是 I2C 设备。
// DLight 单独测试时，AtomS3R 的 Grove 口使用 G2 作为 SDA。
#define SDA_PIN 2

// DLight 单独测试时，AtomS3R 的 Grove 口使用 G1 作为 SCL。
#define SCL_PIN 1

// BH1750FVI 是 DLight 内部的光照芯片。
// 默认 I2C 地址通常是 0x23。如果读不到，可以尝试改成 0x5C。
#define BH1750_ADDR 0x23

// =========================
// 3. 植物状态定义
// =========================

enum PlantState {
  STATE_IDLE = 0,
  STATE_WILTED = 1,
  STATE_NEED_SUN = 2,
  STATE_SUNLIGHT = 3,
  STATE_WALKING = 4
};

// 多久读取一次传感器并请求一次后端。
// 8000ms = 8 秒，课堂演示时能看到变化，同时不会过于频繁请求 Flask。
const unsigned long SENSOR_INTERVAL_MS = 8000;

unsigned long lastSensorTime = 0;

// =========================
// 4. 状态和文案函数
// =========================

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

String getLocalPlantSpeech(PlantState state) {
  /*
    作用：
    当 Wi-Fi 没连上、后端没开、HTTP 请求失败时，仍然返回一句可显示的本地文案。
    这样硬件演示不会因为网络问题完全中断。
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

PlantState getStateFromLux(float lux) {
  /*
    作用：
    把 DLight 读到的 lux 光照值转换成植物状态。

    当前阈值是第一版经验值，需要现场记录后再调整：
    - lux < 50：非常暗，小芽蔫了。
    - 50 <= lux < 300：有一点光，但还想要真实阳光。
    - lux >= 300：认为已经晒到足够明显的光。
  */
  if (lux < 50) {
    return STATE_WILTED;
  }

  if (lux < 300) {
    return STATE_NEED_SUN;
  }

  return STATE_SUNLIGHT;
}

// =========================
// 5. BH1750 / DLight 传感器
// =========================

void writeBH1750Command(byte command) {
  /*
    作用：
    向 BH1750 芯片发送一个控制命令。

    Wire.beginTransmission(address) 表示开始和指定 I2C 地址的设备说话。
    Wire.write(command) 表示发送一个字节命令。
    Wire.endTransmission() 表示结束这次 I2C 通信。
  */
  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(command);
  Wire.endTransmission();
}

void initBH1750() {
  /*
    作用：
    启动 DLight 里的 BH1750 光照芯片。

    0x01：Power On，让芯片开机。
    0x10：Continuous High Resolution Mode，让芯片持续输出较高精度的光照值。
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
    - 正数：实际光照值。
    - -1：读取失败，通常表示线没接好、地址不对、或者传感器没有供电。
  */
  Wire.requestFrom(BH1750_ADDR, 2);

  if (Wire.available() == 2) {
    byte highByte = Wire.read();
    byte lowByte = Wire.read();

    unsigned int rawValue = (highByte << 8) | lowByte;

    // BH1750 官方换算关系约为 raw / 1.2 = lux。
    float lux = rawValue / 1.2;
    return lux;
  }

  return -1;
}

// =========================
// 6. Wi-Fi 和 Flask 请求
// =========================

void connectWiFi() {
  /*
    作用：
    尝试连接 Wi-Fi。

    这里没有无限等待，而是最多等 15 秒。
    原因是课堂演示时如果 Wi-Fi 暂时不可用，程序也应该继续读取光照并使用本地文案。
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
    Serial.println("WiFi connection failed. Local fallback speech will be used.");
  }
}

String askPlantServer(PlantState state, float lux, String motion) {
  /*
    作用：
    把植物状态、光照值、动作状态发给 Flask 后端，并读取后端返回的植物语言。

    输入：
    state：植物状态，例如 STATE_WILTED。
    lux：DLight 读到的光照值。
    motion：动作状态。当前还没有接 IMU，所以先传 "still"。

    输出：
    后端返回的一句植物语言。如果请求失败，返回本地固定文案。
  */
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi is not connected. Use local speech.");
    return getLocalPlantSpeech(state);
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  String stateText = plantStateToText(state);

  String payload = "{";
  payload += "\"state\":\"" + stateText + "\",";
  payload += "\"lux\":" + String(lux, 2) + ",";
  payload += "\"motion\":\"" + motion + "\"";
  payload += "}";

  Serial.print("Send to server: ");
  Serial.println(payload);

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    String response = http.getString();

    Serial.print("HTTP code: ");
    Serial.println(httpCode);
    Serial.print("Server response: ");
    Serial.println(response);

    http.end();

    if (response.length() > 0) {
      return response;
    }

    return getLocalPlantSpeech(state);
  }

  Serial.print("HTTP request failed. Error code: ");
  Serial.println(httpCode);

  http.end();
  return getLocalPlantSpeech(state);
}

// =========================
// 7. S3R 自带彩屏显示
// =========================

void showResultOnS3R(float lux, PlantState state, String speech) {
  /*
    作用：
    在 AtomS3R 自带彩屏上显示当前 lux、状态和后端返回文案。

    注意：
    这个模式不接 OLED，所以彩屏临时承担调试显示功能。
    未来合并版里，彩屏会回到“只画彩色小芽”，文字交给 OLED。
  */
  AtomS3.Display.fillScreen(BLACK);
  AtomS3.Display.setTextColor(WHITE, BLACK);
  AtomS3.Display.setTextSize(1);

  AtomS3.Display.setCursor(6, 8);
  AtomS3.Display.print("City Sprout");

  AtomS3.Display.drawLine(6, 22, 122, 22, DARKGREY);

  AtomS3.Display.setCursor(6, 34);
  AtomS3.Display.print("Lux: ");
  AtomS3.Display.print(lux, 1);

  AtomS3.Display.setCursor(6, 50);
  AtomS3.Display.print("State: ");
  AtomS3.Display.print(plantStateToText(state));

  AtomS3.Display.setCursor(6, 72);

  // 简单按屏幕宽度手动切行。这里每行大约放 18 个字符。
  int lineLength = 0;
  for (int i = 0; i < speech.length(); i++) {
    AtomS3.Display.print(speech.charAt(i));
    lineLength++;

    if (lineLength >= 18 && speech.charAt(i) == ' ') {
      AtomS3.Display.println();
      AtomS3.Display.setCursor(6, AtomS3.Display.getCursorY());
      lineLength = 0;
    }
  }
}

// =========================
// 8. Arduino 标准入口
// =========================

void setup() {
  Serial.begin(115200);
  delay(500);

  auto cfg = M5.config();
  AtomS3.begin(cfg);
  AtomS3.Display.setRotation(0);
  AtomS3.Display.setBrightness(120);

  AtomS3.Display.fillScreen(BLACK);
  AtomS3.Display.setTextColor(WHITE, BLACK);
  AtomS3.Display.setTextSize(1);
  AtomS3.Display.setCursor(6, 8);
  AtomS3.Display.println("DLight + Flask");
  AtomS3.Display.setCursor(6, 28);
  AtomS3.Display.println("Starting...");

  // 启动 I2C 总线，并指定已经验证过的 AtomS3R Grove 引脚。
  Wire.begin(SDA_PIN, SCL_PIN);

  // 启动 DLight 光照传感器。
  initBH1750();

  // 尝试连接 Wi-Fi。失败也不会卡死，后面会使用本地文案。
  connectWiFi();

  Serial.println("DLight + Flask demo started.");
}

void loop() {
  unsigned long now = millis();

  if (now - lastSensorTime < SENSOR_INTERVAL_MS) {
    return;
  }

  lastSensorTime = now;

  float lux = readLightLux();

  if (lux < 0) {
    Serial.println("Failed to read DLight. Check Grove cable and I2C address.");
    AtomS3.Display.fillScreen(BLACK);
    AtomS3.Display.setTextColor(RED, BLACK);
    AtomS3.Display.setTextSize(1);
    AtomS3.Display.setCursor(6, 8);
    AtomS3.Display.println("DLight read failed");
    AtomS3.Display.setCursor(6, 28);
    AtomS3.Display.println("Check cable/address");
    return;
  }

  PlantState state = getStateFromLux(lux);

  // 当前还没有接入 IMU，所以 motion 先固定为 still。
  // 后续加入 AtomS3R 内置 IMU 后，可以把这里替换成 getMotionState()。
  String motion = "still";

  String speech = askPlantServer(state, lux, motion);

  Serial.println("------------------------------");
  Serial.print("Lux: ");
  Serial.println(lux, 2);
  Serial.print("State: ");
  Serial.println(plantStateToText(state));
  Serial.print("Motion: ");
  Serial.println(motion);
  Serial.print("Plant says: ");
  Serial.println(speech);
  Serial.println("------------------------------");

  showResultOnS3R(lux, state, speech);
}
