/*
  City Sprout / 出走小芽
  文件：city_sprout_pahub_main.ino

  这一版是 PaHUB 到货后的“主程序整合版”。

  当前硬件连接方式：
  1. AtomS3R + Atomic Voice Base 作为主控和声音底座。
  2. AtomS3R 的 Grove / HY2.0-4P 主接口接到 PaHUB 主接口。
  3. PaHUB 0 号分接口接 DLight 光照传感器，芯片是 BH1750。
  4. PaHUB 1 号分接口接 Unit OLED 1.3 inch，芯片是 SH1107，黑白屏。
  5. PaHUB 2 号分接口接 Unit ENV-Pro，芯片是 BME688。

  程序做的事情：
  1. 读取光照 lux。
  2. 读取 AtomS3R 内置 IMU，判断 still / walking。
  3. 读取 Atomic Voice Base 麦克风，判断 stable / dynamic。
  4. 读取 ENV-Pro，得到温度、湿度、气压、气体阻值和 IAQ。
  5. 综合光照、运动、声音判断小芽状态和 indoor / outside。
  6. 把所有状态发给 Flask 后端。
  7. 后端返回一句植物文案，OLED 显示这句话。
  8. AtomS3R 自带彩屏显示彩色小芽动画。

  需要的 Arduino 库：
  - M5Unified
  - U8g2
  - BME68x Sensor library
  - M5Unit-ENV

  本机已经检测到这些 ENV-Pro 相关库存在：
  Documents/Arduino/libraries/M5Unit-ENV
  Documents/Arduino/libraries/BME68x_Sensor_library
*/

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <M5Unified.h>
#include <U8g2lib.h>
#include <bme68xLibrary.h>
#include <math.h>

// =========================
// 1. Wi-Fi 和后端地址
// =========================

// 这里填你们现场 Wi-Fi 名称。
// 注意：AtomS3R 和云服务器通信时，只要 Wi-Fi 能上网，就可以访问云服务器。
const char* WIFI_SSID = "DAElab";

// 这里填 Wi-Fi 密码。
// 如果之后要上传 GitHub，建议改成占位符，真实密码只留在本地。
const char* WIFI_PASSWORD = "tjdaelab";

// 云服务器 Flask 后端接口。
// Arduino 会 POST 到 /plant，后端返回一句纯文本植物文案。
const char* SERVER_URL = "http://111.229.81.45:5000/plant";

// =========================
// 2. I2C / PaHUB 设置
// =========================

// AtomS3R Grove 口已经验证过：SDA 是 G2，SCL 是 G1。
// PaHUB、DLight、OLED、ENV-Pro 都通过这两根 I2C 线通信。
#define SDA_PIN 2
#define SCL_PIN 1

// PaHUB 是一个 I2C 多路选择器，常见地址是 0x70。
// 每次访问某个分接口前，都要先告诉 PaHUB：“请打开第几个通道”。
#define PAHUB_ADDR 0x70

// 按你们现在的接线：
// 0 号口 = DLight，1 号口 = OLED，2 号口 = ENV-Pro。
#define PAHUB_CHANNEL_DLIGHT 0
#define PAHUB_CHANNEL_OLED 1
#define PAHUB_CHANNEL_ENV 2

// DLight 光照传感器 BH1750 的 I2C 地址。
// 如果现场读不到，可以尝试改成 0x5C。
#define BH1750_ADDR 0x23

// ENV-Pro / BME688 可能是 0x77，也可能是 0x76。
// M5 官方 ENV_PRO 示例使用 0x77，但现场硬件批次或焊盘设置不同时可能会变成 0x76。
// 所以下面初始化 ENV-Pro 时会两个地址都试，不在这里写死。

// =========================
// 3. OLED 设置
// =========================

// OLED 是 SH1107，物理像素逻辑是 64x128。
// U8G2_R1 表示转成横屏 128x64，用来显示英文短句更合适。
// 如果你们现场发现文字方向反了，把 U8G2_R1 改成 U8G2_R3。
U8G2_SH1107_64X128_F_HW_I2C oled(
  U8G2_R1,
  U8X8_PIN_NONE,
  SCL_PIN,
  SDA_PIN
);

// =========================
// 4. 状态和阈值
// =========================

enum PlantState { STATE_IDLE, STATE_WILTED, STATE_NEED_SUN, STATE_SUNLIGHT, STATE_WALKING };
enum MotionState { MOTION_STILL, MOTION_ACTIVE };
enum SoundState { SOUND_UNKNOWN, SOUND_STABLE, SOUND_DYNAMIC };
enum PlaceState { PLACE_UNKNOWN, PLACE_INDOOR, PLACE_OUTSIDE };

// 光照阈值。单位是 lux。
// 这些值先用于 Demo，后续应该根据你们教室、窗边、室外实测再微调。
const float LUX_WILTED_MAX = 50.0;
const float LUX_NEED_SUN_MAX = 300.0;
const float LUX_OUTSIDE_HINT = 800.0;

// IMU 运动阈值。
// motionStrength 是“当前加速度总量离 1g 有多远”。
// 静止时理论上接近 0，拿起来走动时会变大。
const float ACTIVE_THRESHOLD = 0.06;
const unsigned long ACTIVE_HOLD_TIME_MS = 1500;

// 麦克风环境声判断参数。
// 这里不是识别具体声音，只判断一段时间内声音是否变化明显。
const int SOUND_SAMPLE_RATE = 16000;
const int SOUND_SAMPLE_COUNT = 256;
const int SOUND_HISTORY_SIZE = 12;
const unsigned long SOUND_INTERVAL_MS = 500;
const float SOUND_DYNAMIC_RANGE_THRESHOLD = 0.020;
const float SOUND_DYNAMIC_VARIANCE_THRESHOLD = 0.00008;

// 各任务刷新间隔。
const unsigned long SENSOR_INTERVAL_MS = 300;
const unsigned long ENV_INTERVAL_MS = 3000;
const unsigned long DRAW_INTERVAL_MS = 120;
const unsigned long OLED_INTERVAL_MS = 1200;
const unsigned long SERVER_INTERVAL_MS = 8000;
const unsigned long PRINT_INTERVAL_MS = 1000;

// =========================
// 5. 全局变量
// =========================

Bme68x envSensor;

PlantState currentState = STATE_IDLE;
MotionState currentMotion = MOTION_STILL;
SoundState currentSound = SOUND_UNKNOWN;
PlaceState currentPlace = PLACE_UNKNOWN;

int16_t micBuffer[SOUND_SAMPLE_COUNT];
float soundHistory[SOUND_HISTORY_SIZE];
int soundHistoryIndex = 0;
int soundHistoryCount = 0;
bool micReady = false;

bool envReady = false;
bool envHasData = false;
uint8_t envI2CAddress = BME68X_I2C_ADDR_HIGH;
float lastTemperature = NAN;
float lastHumidity = NAN;
float lastPressure = NAN;
float lastGasResistance = NAN;
float lastIAQ = NAN;
int lastIAQAccuracy = 0;

float lastLux = -1.0;
float lastMotionStrength = 0.0;
float lastSoundLevel = 0.0;
float lastSoundRange = 0.0;
float lastSoundVariance = 0.0;
String lastSpeech = "Waiting for the city.";

unsigned long lastActiveTime = 0;
unsigned long lastSensorTime = 0;
unsigned long lastEnvTime = 0;
unsigned long lastDrawTime = 0;
unsigned long lastOledTime = 0;
unsigned long lastServerTime = 0;
unsigned long lastPrintTime = 0;
unsigned long lastSoundTime = 0;

int frame = 0;

uint16_t COLOR_BG;
uint16_t COLOR_STEM;
uint16_t COLOR_LEAF;
uint16_t COLOR_LEAF_LIGHT;
uint16_t COLOR_LEAF_DARK;
uint16_t COLOR_SUN;
uint16_t COLOR_TEXT;
uint16_t COLOR_POT;

// =========================
// 6. PaHUB 基础函数
// =========================

bool selectPaHubChannel(uint8_t channel) {
  /*
    作用：
    选择 PaHUB 的某一个分接口。

    为什么需要这个函数：
    DLight、OLED、ENV-Pro 都接在 PaHUB 后面。
    主控每次只能和 PaHUB 当前打开的通道通信。
    所以读 DLight 前要选 0，刷 OLED 前要选 1，读 ENV-Pro 前要选 2。

    输入：
    channel：0 到 5，表示 PaHUB 上的分接口编号。

    输出：
    true 表示 PaHUB 接受了切换命令。
    false 表示 PaHUB 没有响应，通常是线没接好或地址不对。
  */
  if (channel > 5) return false;

  Wire.beginTransmission(PAHUB_ADDR);
  Wire.write(1 << channel);
  byte error = Wire.endTransmission();
  return error == 0;
}

bool i2cDeviceExists(uint8_t address) {
  /*
    作用：
    检查当前 PaHUB 通道上某个 I2C 地址有没有设备回应。

    注意：
    这个函数只检查“有没有回应”，不读取传感器数据。
    调用前必须先 selectPaHubChannel(...)，否则检查的是错误通道。
  */
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void printI2CScanForCurrentChannel(String label) {
  /*
    作用：
    把当前 PaHUB 通道上能看到的 I2C 地址打印出来。

    为什么加这个：
    你们现在有三个分接口。如果 ENV-Pro 还是 nan，
    串口里看到扫描结果就能判断是“没有接通”，还是“地址不对/库没读到”。
  */
  Serial.print(label);
  Serial.print(" I2C scan:");

  bool foundAny = false;
  for (uint8_t address = 1; address < 127; address++) {
    if (i2cDeviceExists(address)) {
      Serial.print(" 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      foundAny = true;
    }
  }

  if (!foundAny) {
    Serial.print(" no device");
  }

  Serial.println();
}

// =========================
// 7. 文本转换
// =========================

String plantStateToText(PlantState state) {
  if (state == STATE_WILTED) return "wilted";
  if (state == STATE_NEED_SUN) return "need_sun";
  if (state == STATE_SUNLIGHT) return "sunlight";
  if (state == STATE_WALKING) return "walking";
  return "idle";
}

String plantStateToTitle(PlantState state) {
  if (state == STATE_WILTED) return "Wilted";
  if (state == STATE_NEED_SUN) return "Need Sun";
  if (state == STATE_SUNLIGHT) return "Sunlight";
  if (state == STATE_WALKING) return "Walking";
  return "Idle";
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

String floatOrNull(float value, int decimals) {
  /*
    作用：
    生成 JSON 时使用。
    如果 ENV-Pro 还没有数据，就返回 null。
    如果有数据，就返回指定小数位的数字字符串。
  */
  if (isnan(value)) return "null";
  return String(value, decimals);
}

String getLocalPlantSpeech(PlantState state) {
  if (state == STATE_WALKING) return "Are we going outside?";
  if (state == STATE_WILTED) return "I only saw screen light today.";
  if (state == STATE_NEED_SUN) return "Please take me to real sun.";
  if (state == STATE_SUNLIGHT) return "Sunlight found. I feel alive.";
  return "I am waiting for light.";
}

// =========================
// 8. DLight / BH1750
// =========================

void writeBH1750Command(byte command) {
  selectPaHubChannel(PAHUB_CHANNEL_DLIGHT);
  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(command);
  Wire.endTransmission();
}

void initBH1750() {
  /*
    BH1750 初始化步骤：
    0x01 = power on，打开传感器。
    0x10 = continuous high resolution mode，让它持续测光。
  */
  selectPaHubChannel(PAHUB_CHANNEL_DLIGHT);
  writeBH1750Command(0x01);
  delay(10);
  writeBH1750Command(0x10);
  delay(200);
  Serial.println("DLight on PaHUB channel 0 initialized.");
}

float readLightLux() {
  selectPaHubChannel(PAHUB_CHANNEL_DLIGHT);
  Wire.requestFrom(BH1750_ADDR, 2);

  if (Wire.available() == 2) {
    byte highByte = Wire.read();
    byte lowByte = Wire.read();
    unsigned int rawValue = (highByte << 8) | lowByte;
    return rawValue / 1.2;
  }

  return -1.0;
}

PlantState getStateFromLux(float lux) {
  if (lux < 0) return STATE_IDLE;
  if (lux < LUX_WILTED_MAX) return STATE_WILTED;
  if (lux < LUX_NEED_SUN_MAX) return STATE_NEED_SUN;
  return STATE_SUNLIGHT;
}

// =========================
// 9. IMU 运动检测
// =========================

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

// =========================
// 10. Atomic Voice Base 麦克风
// =========================

void initMic() {
  /*
    这里沿用 voice_base_mic_test.ino 中已经验证可用的设置。
    Atomic Voice Base 的麦克风需要 M5Unified 初始化底座音频硬件。
    同时我们先关闭 Speaker，避免扬声器占用音频资源。
  */
  M5.Speaker.end();

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

  if (soundHistoryCount < SOUND_HISTORY_SIZE) {
    soundHistoryCount++;
  }
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
  float sum = 0.0;

  for (int i = 0; i < soundHistoryCount; i++) {
    float level = soundHistory[i];
    if (level < minLevel) minLevel = level;
    if (level > maxLevel) maxLevel = level;
    sum += level;
  }

  float mean = sum / soundHistoryCount;
  float varianceSum = 0.0;

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

// =========================
// 11. ENV-Pro / BME688
// =========================

void initEnvPro() {
  /*
    初始化 ENV-Pro。
    这一版不用 BSEC 的 IAQ 异步算法，改用 BME68x forced mode 同步读数。

    为什么这样改：
    - BSEC 的 IAQ 数据需要预热，而且它会自己安排测量时间。
    - PaHUB 场景下我们会频繁在 DLight / OLED / ENV-Pro 之间切通道。
    - forced mode 是“选中 2 号口 -> 立刻测量 -> 立刻读取”，更适合先把 Demo 跑稳。

    当前能稳定拿到：
    - temperature：温度
    - humidity：湿度
    - pressure：气压
    - gas_resistance：气体阻值，可作为空气变化趋势参考

    IAQ 暂时保留为 nan，后续真要空气质量评分时再单独接回 BSEC。
  */
  selectPaHubChannel(PAHUB_CHANNEL_ENV);
  printI2CScanForCurrentChannel("PaHUB channel 2 / ENV-Pro");

  bool hasHighAddress = i2cDeviceExists(BME68X_I2C_ADDR_HIGH);
  bool hasLowAddress = i2cDeviceExists(BME68X_I2C_ADDR_LOW);

  if (hasHighAddress) {
    envI2CAddress = BME68X_I2C_ADDR_HIGH;
  } else if (hasLowAddress) {
    envI2CAddress = BME68X_I2C_ADDR_LOW;
  } else {
    envReady = false;
    Serial.println("ENV-Pro not found on PaHUB channel 2. Check cable/port.");
    return;
  }

  Serial.print("ENV-Pro detected at address 0x");
  Serial.println(envI2CAddress, HEX);

  envSensor.begin(envI2CAddress, Wire);

  if (envSensor.checkStatus() == BME68X_ERROR) {
    envReady = false;
    Serial.println("ENV-Pro begin failed on PaHUB channel 2.");
    Serial.println("BME68x error: " + envSensor.statusString());
    return;
  }

  if (envSensor.checkStatus() == BME68X_WARNING) {
    Serial.println("BME68x warning: " + envSensor.statusString());
  }

  envSensor.setTPH();
  envSensor.setHeaterProf(300, 100);

  envReady = true;
  Serial.println("ENV-Pro on PaHUB channel 2 initialized.");
  Serial.println("ENV-Pro mode: BME68x forced mode, reading every 3 seconds.");
}

void updateEnvPro() {
  if (!envReady) return;

  unsigned long now = millis();
  if (now - lastEnvTime < ENV_INTERVAL_MS) return;
  lastEnvTime = now;

  selectPaHubChannel(PAHUB_CHANNEL_ENV);

  bme68xData data;

  envSensor.setOpMode(BME68X_FORCED_MODE);
  delayMicroseconds(envSensor.getMeasDur());

  if (envSensor.fetchData()) {
    envSensor.getData(data);

    lastTemperature = data.temperature;
    lastHumidity = data.humidity;
    lastPressure = data.pressure / 100.0;
    lastGasResistance = data.gas_resistance;
    lastIAQ = NAN;
    lastIAQAccuracy = 0;
    envHasData = true;

    Serial.print("ENV-Pro data: temp=");
    Serial.print(lastTemperature);
    Serial.print(" hum=");
    Serial.print(lastHumidity);
    Serial.print(" press=");
    Serial.print(lastPressure);
    Serial.print(" gas=");
    Serial.println(lastGasResistance);
  } else {
    Serial.println("ENV-Pro fetchData() returned no data.");
  }
}

// =========================
// 12. 综合状态判断
// =========================

PlaceState updatePlaceState(float lux, MotionState motion, SoundState sound) {
  int outsideScore = 0;

  if (lux >= LUX_OUTSIDE_HINT) outsideScore++;
  if (motion == MOTION_ACTIVE) outsideScore++;
  if (sound == SOUND_DYNAMIC) outsideScore++;

  if (outsideScore >= 2) return PLACE_OUTSIDE;

  if (lux < LUX_NEED_SUN_MAX &&
      motion == MOTION_STILL &&
      sound == SOUND_STABLE) {
    return PLACE_INDOOR;
  }

  return PLACE_UNKNOWN;
}

// =========================
// 13. Wi-Fi 和后端
// =========================

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

String askPlantServer() {
  if (WiFi.status() != WL_CONNECTED) {
    return getLocalPlantSpeech(currentState);
  }

  HTTPClient http;
  WiFiClient plainClient;
  WiFiClientSecure secureClient;

  if (String(SERVER_URL).startsWith("https://")) {
    secureClient.setInsecure();
    http.begin(secureClient, SERVER_URL);
  } else {
    http.begin(plainClient, SERVER_URL);
  }

  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"state\":\"" + plantStateToText(currentState) + "\",";
  payload += "\"lux\":" + String(lastLux, 2) + ",";
  payload += "\"motion\":\"" + motionToText(currentMotion) + "\",";
  payload += "\"sound_state\":\"" + soundToText(currentSound) + "\",";
  payload += "\"sound_level\":" + String(lastSoundLevel, 5) + ",";
  payload += "\"sound_range\":" + String(lastSoundRange, 5) + ",";
  payload += "\"sound_variance\":" + String(lastSoundVariance, 7) + ",";
  payload += "\"place\":\"" + placeToText(currentPlace) + "\",";
  payload += "\"env_ready\":" + String(envReady && envHasData ? "true" : "false") + ",";
  payload += "\"temperature_c\":" + floatOrNull(lastTemperature, 2) + ",";
  payload += "\"humidity_percent\":" + floatOrNull(lastHumidity, 2) + ",";
  payload += "\"pressure_hpa\":" + floatOrNull(lastPressure, 2) + ",";
  payload += "\"gas_resistance_ohm\":" + floatOrNull(lastGasResistance, 2) + ",";
  payload += "\"iaq\":" + floatOrNull(lastIAQ, 2) + ",";
  payload += "\"iaq_accuracy\":" + String(lastIAQAccuracy);
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
  }

  http.end();
  return getLocalPlantSpeech(currentState);
}

// =========================
// 14. OLED 显示
// =========================

void wrapTextForOLED(String text, String lines[], int maxLines) {
  const int MAX_CHARS_PER_LINE = 18;

  for (int i = 0; i < maxLines; i++) {
    lines[i] = "";
  }

  int currentLine = 0;
  String currentWord = "";

  for (int i = 0; i <= text.length(); i++) {
    char c = (i < text.length()) ? text.charAt(i) : ' ';

    if (c == ' ') {
      if (currentWord.length() == 0) continue;

      String candidate = lines[currentLine];
      if (candidate.length() > 0) candidate += " ";
      candidate += currentWord;

      if (candidate.length() <= MAX_CHARS_PER_LINE) {
        lines[currentLine] = candidate;
      } else {
        currentLine++;
        if (currentLine >= maxLines) return;
        lines[currentLine] = currentWord;
      }

      currentWord = "";
    } else {
      currentWord += c;
    }
  }
}

void showTextOnOLED() {
  selectPaHubChannel(PAHUB_CHANNEL_OLED);

  String lines[2];
  wrapTextForOLED(lastSpeech, lines, 2);

  String envText = "ENV:wait";
  if (envHasData && !isnan(lastTemperature) && !isnan(lastHumidity)) {
    envText = String(lastTemperature, 1) + "C " + String(lastHumidity, 0) + "%";
  }

  oled.clearBuffer();
  oled.setDrawColor(0);
  oled.drawBox(0, 0, 128, 64);
  oled.setDrawColor(1);
  oled.setFont(u8g2_font_5x7_tr);

  String line0 = plantStateToTitle(currentState) + " " + placeToText(currentPlace);
  String line1 = String((int)lastLux) + "lx " + motionToText(currentMotion) + " " + soundToText(currentSound);

  oled.drawStr(0, 7, line0.c_str());
  oled.drawStr(0, 16, line1.c_str());
  oled.drawStr(0, 25, envText.c_str());
  oled.drawLine(0, 29, 127, 29);

  for (int i = 0; i < 2; i++) {
    if (lines[i].length() > 0) {
      oled.drawStr(0, 43 + i * 11, lines[i].c_str());
    }
  }

  oled.sendBuffer();
}

// =========================
// 15. S3R 彩屏小芽
// =========================

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

void drawSproutOnS3R() {
  M5.Display.fillScreen(COLOR_BG);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);

  // S3R 彩屏主要显示小芽动画，不再在左上角堆 lux / motion / sound 小字。
  // 这些调试小字已经移动到 OLED，避免彩屏画面太杂。
  // 右上角保留 place，因为它是演示时最容易解释的综合判断。
  M5.Display.setCursor(76, 4);
  M5.Display.print(placeToText(currentPlace));

  if (!isnan(lastTemperature)) {
    M5.Display.setCursor(76, 16);
    M5.Display.print(lastTemperature, 1);
    M5.Display.print("C");
  }

  int phase = frame % 20;
  int sway = phase < 5 ? -2 : phase < 10 ? -1 : phase < 15 ? 1 : 2;
  int breathe = (frame % 30 < 15) ? 0 : 2;
  int centerX = 64;
  int baseY = 112;
  int topY = 44 + breathe;
  int leafY = 70;
  int leafDrop = 0;
  int plantSway = currentMotion == MOTION_ACTIVE ? sway * 3 : sway;
  uint16_t leafColor = COLOR_LEAF;

  if (currentState == STATE_WILTED) {
    topY = 62;
    leafY = 80;
    leafDrop = 12;
    plantSway = 0;
    leafColor = COLOR_LEAF_DARK;
  } else if (currentState == STATE_NEED_SUN) {
    topY = 52 + breathe;
    leafDrop = 2;
  } else if (currentState == STATE_SUNLIGHT) {
    topY = 32 + breathe;
    leafY = 62;
    leafDrop = -6;
    leafColor = COLOR_LEAF_LIGHT;
    M5.Display.fillCircle(104, 22, 7, COLOR_SUN);
  } else if (currentState == STATE_WALKING) {
    M5.Display.drawLine(10, 48, 30, 48, COLOR_TEXT);
    M5.Display.drawLine(96, 58, 118, 58, COLOR_TEXT);
  }

  if (currentSound == SOUND_DYNAMIC) {
    M5.Display.drawCircle(18, 42, 3, COLOR_TEXT);
    M5.Display.drawCircle(18, 42, 7, COLOR_TEXT);
  }

  M5.Display.drawLine(centerX - 2, baseY, centerX + plantSway, topY, COLOR_STEM);
  M5.Display.drawLine(centerX - 1, baseY, centerX + plantSway + 1, topY, COLOR_STEM);
  M5.Display.drawLine(centerX, baseY, centerX + plantSway + 2, topY, COLOR_STEM);
  M5.Display.fillEllipse(centerX - 26 + plantSway, leafY + leafDrop, 24, 11, leafColor);
  M5.Display.fillEllipse(centerX + 26 + plantSway, leafY + leafDrop + 4, 24, 11, leafColor);
  M5.Display.fillEllipse(centerX + plantSway + 8, topY + 4, currentState == STATE_SUNLIGHT ? 12 : 9, currentState == STATE_SUNLIGHT ? 6 : 5, leafColor);
  M5.Display.fillRoundRect(42, 112, 44, 10, 5, COLOR_POT);
  M5.Display.drawRoundRect(40, 110, 48, 14, 5, COLOR_TEXT);
}

// =========================
// 16. 串口调试
// =========================

void printDebugLine() {
  Serial.print("Lux:");
  Serial.print(lastLux, 2);
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
  Serial.print(placeToText(currentPlace));

  Serial.print(" | temp:");
  Serial.print(lastTemperature);
  Serial.print(" hum:");
  Serial.print(lastHumidity);
  Serial.print(" press:");
  Serial.print(lastPressure);
  Serial.print(" gas:");
  Serial.print(lastGasResistance);
  Serial.print(" iaq:");
  Serial.print(lastIAQ);
  Serial.print(" acc:");
  Serial.println(lastIAQAccuracy);
}

// =========================
// 17. Arduino 标准入口
// =========================

void setup() {
  Serial.begin(115200);
  delay(1000);

  auto cfg = M5.config();
  cfg.external_speaker.atomic_echo = true;

  // Atomic Voice Base 需要打开 atomic_echo 配置，麦克风才更稳定。
  cfg.external_speaker.atomic_echo = true;

  M5.begin(cfg);
  M5.Display.setBrightness(120);

  initColors();

  Wire.begin(SDA_PIN, SCL_PIN);

  initBH1750();

  selectPaHubChannel(PAHUB_CHANNEL_OLED);
  oled.begin();
  oled.setContrast(255);
  oled.clearDisplay();
  oled.clearBuffer();
  oled.setDrawColor(0);
  oled.drawBox(0, 0, 128, 64);
  oled.sendBuffer();
  delay(80);
  oled.clearBuffer();
  oled.sendBuffer();
  oled.setDrawColor(1);
  showTextOnOLED();
  Serial.println("OLED on PaHUB channel 1 initialized.");

  initEnvPro();
  initMic();
  connectWiFi();

  Serial.println("City Sprout PaHUB main started.");
}

void loop() {
  M5.update();
  unsigned long now = millis();

  // ENV-Pro 现在每 3 秒同步读一次；函数内部会自己判断是否到时间。
  updateEnvPro();

  if (now - lastSoundTime >= SOUND_INTERVAL_MS) {
    currentSound = updateSoundState();
    lastSoundTime = now;
  }

  if (now - lastSensorTime >= SENSOR_INTERVAL_MS) {
    lastLux = readLightLux();

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
    lastSensorTime = now;
  }

  if (now - lastDrawTime >= DRAW_INTERVAL_MS) {
    drawSproutOnS3R();
    frame++;
    lastDrawTime = now;
  }

  if (now - lastOledTime >= OLED_INTERVAL_MS) {
    showTextOnOLED();
    lastOledTime = now;
  }

  if (lastLux >= 0 && now - lastServerTime >= SERVER_INTERVAL_MS) {
    lastSpeech = askPlantServer();
    Serial.print("Plant says: ");
    Serial.println(lastSpeech);
    lastServerTime = now;
  }

  if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
    printDebugLine();
    lastPrintTime = now;
  }
}
