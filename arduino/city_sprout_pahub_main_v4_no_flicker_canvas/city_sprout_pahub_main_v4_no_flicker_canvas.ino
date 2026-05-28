#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <M5Unified.h>
#include <U8g2lib.h>
#include <bme68xLibrary.h>
#include <math.h>

const char* WIFI_SSID = "DAElab";
const char* WIFI_PASSWORD = "tjdaelab";
const char* SERVER_URL = "http://111.229.81.45:5000/plant";

#define SDA_PIN 2
#define SCL_PIN 1

#define PAHUB_ADDR 0x70

#define PAHUB_CHANNEL_DLIGHT 0
#define PAHUB_CHANNEL_OLED 1
#define PAHUB_CHANNEL_ENV 2

#define BH1750_ADDR 0x23

U8G2_SH1107_64X128_F_HW_I2C oled(
  U8G2_R1,
  U8X8_PIN_NONE,
  SCL_PIN,
  SDA_PIN
);

enum PlantState { STATE_IDLE, STATE_WILTED, STATE_NEED_SUN, STATE_SUNLIGHT, STATE_WALKING };
enum MotionState { MOTION_STILL, MOTION_ACTIVE };
enum SoundState { SOUND_UNKNOWN, SOUND_QUIET, SOUND_ACTIVE, SOUND_INTENSE };
enum PlaceState { PLACE_UNKNOWN, PLACE_INDOOR, PLACE_OUTSIDE };

const float LUX_WILTED_MAX = 50.0;
const float LUX_NEED_SUN_MAX = 300.0;
const float LUX_OUTSIDE_HINT = 800.0;

const float ACTIVE_THRESHOLD = 0.06;
const unsigned long ACTIVE_HOLD_TIME_MS = 1500;

const int SOUND_SAMPLE_RATE = 16000;
const int SOUND_SAMPLE_COUNT = 512;
const int SOUND_HISTORY_SIZE = 8;
const unsigned long SOUND_INTERVAL_MS = 300;

const float SOUND_QUIET_LEVEL_THRESHOLD = 0.012;
const float SOUND_ACTIVE_RANGE_THRESHOLD = 0.015;
const float SOUND_INTENSE_LEVEL_THRESHOLD = 0.075;
const float SOUND_INTENSE_PEAK_THRESHOLD = 0.40;
const float SOUND_INTENSE_RANGE_THRESHOLD = 0.055;
const float SOUND_INTENSE_VARIANCE_THRESHOLD = 0.00020;

const unsigned long SENSOR_INTERVAL_MS = 300;
const unsigned long ENV_INTERVAL_MS = 3000;
const unsigned long DRAW_INTERVAL_MS = 120;
const unsigned long OLED_INTERVAL_MS = 1200;
const unsigned long SERVER_INTERVAL_MS = 20000;
const unsigned long PRINT_INTERVAL_MS = 1000;

Bme68x envSensor;

PlantState currentState = STATE_IDLE;
MotionState currentMotion = MOTION_STILL;
SoundState currentSound = SOUND_UNKNOWN;
PlaceState currentPlace = PLACE_UNKNOWN;

struct RealtimePlantResponse {
  float calm = 70.0;
  float curiosity = 20.0;
  float stress = 0.0;
};

RealtimePlantResponse realtimePlant;

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
float lastSoundMean = 0.0;
float lastSoundPeak = 0.0;
float lastSoundRange = 0.0;
float lastSoundVariance = 0.0;
String lastSpeech = "Waiting for the city.";

bool demoAutoMode = false;
SoundState forcedSound = SOUND_UNKNOWN;
unsigned long forcedSoundUntil = 0;

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
uint16_t COLOR_ALERT;
uint16_t COLOR_ACTIVE;
uint16_t COLOR_FLOWER;

M5Canvas sproutCanvas(&M5.Display);

bool selectPaHubChannel(uint8_t channel) {
  if (channel > 5) return false;

  Wire.beginTransmission(PAHUB_ADDR);
  Wire.write(1 << channel);
  byte error = Wire.endTransmission();
  return error == 0;
}

bool i2cDeviceExists(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void printI2CScanForCurrentChannel(String label) {
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
  if (sound == SOUND_QUIET) return "quiet";
  if (sound == SOUND_ACTIVE) return "active";
  if (sound == SOUND_INTENSE) return "intense";
  return "unknown";
}

String placeToText(PlaceState place) {
  if (place == PLACE_INDOOR) return "indoor";
  if (place == PLACE_OUTSIDE) return "outside";
  return "unknown";
}

String floatOrNull(float value, int decimals) {
  if (isnan(value)) return "null";
  return String(value, decimals);
}

String getLocalPlantSpeech(PlantState state) {
  if (currentSound == SOUND_INTENSE) return "That was loud. I feel startled.";
  if (currentSound == SOUND_ACTIVE) return "I hear you. I am curious.";
  if (currentSound == SOUND_QUIET) return "It is quiet. I feel calm.";

  if (state == STATE_WALKING) return "Are we going outside?";
  if (state == STATE_WILTED) return "I only saw screen light today.";
  if (state == STATE_NEED_SUN) return "Please take me to real sun.";
  if (state == STATE_SUNLIGHT) return "Sunlight found. I feel alive.";
  return "I am waiting for light.";
}

void writeBH1750Command(byte command) {
  selectPaHubChannel(PAHUB_CHANNEL_DLIGHT);
  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(command);
  Wire.endTransmission();
}

void initBH1750() {
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

void initMic() {
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

void calculateSoundFeaturesFromBuffer() {
  double sumSquares = 0.0;
  int16_t peakAbs = 0;

  for (int i = 0; i < SOUND_SAMPLE_COUNT; i++) {
    int16_t raw = micBuffer[i];
    float sample = raw / 32768.0;
    sumSquares += sample * sample;

    int16_t absRaw = abs(raw);
    if (absRaw > peakAbs) peakAbs = absRaw;
  }

  lastSoundLevel = sqrt(sumSquares / SOUND_SAMPLE_COUNT);
  lastSoundPeak = peakAbs / 32768.0;
}

void pushSoundHistory(float level) {
  soundHistory[soundHistoryIndex] = level;
  soundHistoryIndex = (soundHistoryIndex + 1) % SOUND_HISTORY_SIZE;

  if (soundHistoryCount < SOUND_HISTORY_SIZE) {
    soundHistoryCount++;
  }
}

void updateSoundStatsFromHistory() {
  if (soundHistoryCount <= 0) {
    lastSoundMean = 0.0;
    lastSoundRange = 0.0;
    lastSoundVariance = 0.0;
    return;
  }

  float minLevel = soundHistory[0];
  float maxLevel = soundHistory[0];
  float sum = 0.0;

  for (int i = 0; i < soundHistoryCount; i++) {
    float level = soundHistory[i];
    if (level < minLevel) minLevel = level;
    if (level > maxLevel) maxLevel = level;
    sum += level;
  }

  lastSoundMean = sum / soundHistoryCount;

  float varianceSum = 0.0;
  for (int i = 0; i < soundHistoryCount; i++) {
    float diff = soundHistory[i] - lastSoundMean;
    varianceSum += diff * diff;
  }

  lastSoundRange = maxLevel - minLevel;
  lastSoundVariance = varianceSum / soundHistoryCount;
}

SoundState updateSoundState() {
  if (!micReady || !M5.Mic.isEnabled()) return SOUND_UNKNOWN;

  bool recorded = M5.Mic.record(micBuffer, SOUND_SAMPLE_COUNT, SOUND_SAMPLE_RATE);
  if (!recorded) return SOUND_UNKNOWN;

  calculateSoundFeaturesFromBuffer();
  pushSoundHistory(lastSoundLevel);
  updateSoundStatsFromHistory();

  if (soundHistoryCount < 3) return SOUND_UNKNOWN;

  if (lastSoundPeak > SOUND_INTENSE_PEAK_THRESHOLD ||
      lastSoundLevel > SOUND_INTENSE_LEVEL_THRESHOLD ||
      lastSoundRange > SOUND_INTENSE_RANGE_THRESHOLD ||
      lastSoundVariance > SOUND_INTENSE_VARIANCE_THRESHOLD) {
    return SOUND_INTENSE;
  }

  if (lastSoundMean < SOUND_QUIET_LEVEL_THRESHOLD &&
      lastSoundRange < SOUND_ACTIVE_RANGE_THRESHOLD) {
    return SOUND_QUIET;
  }

  return SOUND_ACTIVE;
}

float approachValue(float currentValue, float targetValue, float factor) {
  return currentValue + (targetValue - currentValue) * factor;
}

void updateRealtimePlantFromSound(SoundState sound) {
  float targetCalm = 55.0;
  float targetCuriosity = 25.0;
  float targetStress = 5.0;

  if (sound == SOUND_QUIET) {
    targetCalm = 92.0;
    targetCuriosity = 12.0;
    targetStress = 0.0;
  } else if (sound == SOUND_ACTIVE) {
    targetCalm = 45.0;
    targetCuriosity = 88.0;
    targetStress = 10.0;
  } else if (sound == SOUND_INTENSE) {
    targetCalm = 10.0;
    targetCuriosity = 35.0;
    targetStress = 96.0;
  }

  realtimePlant.calm = approachValue(realtimePlant.calm, targetCalm, 0.28);
  realtimePlant.curiosity = approachValue(realtimePlant.curiosity, targetCuriosity, 0.30);
  realtimePlant.stress = approachValue(realtimePlant.stress, targetStress, 0.34);

  realtimePlant.calm = constrain(realtimePlant.calm, 0.0, 100.0);
  realtimePlant.curiosity = constrain(realtimePlant.curiosity, 0.0, 100.0);
  realtimePlant.stress = constrain(realtimePlant.stress, 0.0, 100.0);
}

SoundState getAutoDemoSound() {
  unsigned long t = millis() % 42000;

  if (t < 12000) return SOUND_QUIET;
  if (t < 26000) return SOUND_ACTIVE;
  if (t < 34000) return SOUND_INTENSE;
  return SOUND_QUIET;
}

SoundState getEffectiveSoundState(SoundState realSound) {
  if (demoAutoMode) {
    return getAutoDemoSound();
  }

  if (forcedSound != SOUND_UNKNOWN && millis() < forcedSoundUntil) {
    return forcedSound;
  }

  return realSound;
}

void resetRealtimePlant() {
  realtimePlant.calm = 70.0;
  realtimePlant.curiosity = 20.0;
  realtimePlant.stress = 0.0;
}

void printDemoHelp() {
  Serial.println("Demo commands:");
  Serial.println("  live     : use real microphone");
  Serial.println("  auto     : auto demo quiet -> active -> intense");
  Serial.println("  quiet    : force quiet for 8s");
  Serial.println("  active   : force active for 8s");
  Serial.println("  intense  : force intense for 5s");
  Serial.println("  reset    : reset plant emotion");
  Serial.println("  help     : show commands");
}

void handleSerialDemoCommand() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "live") {
    demoAutoMode = false;
    forcedSound = SOUND_UNKNOWN;
    Serial.println("DEMO: live mode");
  } else if (cmd == "auto") {
    demoAutoMode = true;
    forcedSound = SOUND_UNKNOWN;
    resetRealtimePlant();
    Serial.println("DEMO: auto mode");
  } else if (cmd == "quiet") {
    demoAutoMode = false;
    forcedSound = SOUND_QUIET;
    forcedSoundUntil = millis() + 8000;
    Serial.println("DEMO: force quiet");
  } else if (cmd == "active") {
    demoAutoMode = false;
    forcedSound = SOUND_ACTIVE;
    forcedSoundUntil = millis() + 8000;
    Serial.println("DEMO: force active");
  } else if (cmd == "intense") {
    demoAutoMode = false;
    forcedSound = SOUND_INTENSE;
    forcedSoundUntil = millis() + 5000;
    Serial.println("DEMO: force intense");
  } else if (cmd == "reset") {
    resetRealtimePlant();
    Serial.println("DEMO: reset plant");
  } else if (cmd == "help") {
    printDemoHelp();
  } else {
    Serial.println("Unknown command. Type help.");
  }
}

void initEnvPro() {
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

PlaceState updatePlaceState(float lux, MotionState motion, SoundState sound) {
  int outsideScore = 0;

  if (lux >= LUX_OUTSIDE_HINT) outsideScore++;
  if (motion == MOTION_ACTIVE) outsideScore++;
  if (sound == SOUND_ACTIVE || sound == SOUND_INTENSE) outsideScore++;

  if (outsideScore >= 2) return PLACE_OUTSIDE;

  if (lux < LUX_NEED_SUN_MAX &&
      motion == MOTION_STILL &&
      (sound == SOUND_QUIET || sound == SOUND_UNKNOWN)) {
    return PLACE_INDOOR;
  }

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
  payload += "\"sound_mean\":" + String(lastSoundMean, 5) + ",";
  payload += "\"sound_peak\":" + String(lastSoundPeak, 5) + ",";
  payload += "\"sound_range\":" + String(lastSoundRange, 5) + ",";
  payload += "\"sound_variance\":" + String(lastSoundVariance, 7) + ",";
  payload += "\"calm\":" + String(realtimePlant.calm, 1) + ",";
  payload += "\"curiosity\":" + String(realtimePlant.curiosity, 1) + ",";
  payload += "\"stress\":" + String(realtimePlant.stress, 1) + ",";
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

String toOLEDAscii(String text) {
  String output = "";

  for (int i = 0; i < text.length(); i++) {
    char c = text.charAt(i);

    if (c >= 32 && c <= 126) {
      output += c;
    } else {
      output += ' ';
    }
  }

  output.trim();
  return output;
}

void showTextOnOLED() {
  selectPaHubChannel(PAHUB_CHANNEL_OLED);

  String speechSafe = toOLEDAscii(lastSpeech);
  String lines[2];
  wrapTextForOLED(speechSafe, lines, 2);

  String envText = "ENV wait";
  if (envHasData && !isnan(lastTemperature) && !isnan(lastHumidity)) {
    envText = String(lastTemperature, 1) + "C " + String(lastHumidity, 0) + "%";
  }

  String line0 = plantStateToTitle(currentState) + " " + placeToText(currentPlace);
  String line1 = String((int)lastLux) + "lx " + motionToText(currentMotion) + " " + soundToText(currentSound);

  if (line0.length() > 20) line0 = line0.substring(0, 20);
  if (line1.length() > 20) line1 = line1.substring(0, 20);
  if (envText.length() > 20) envText = envText.substring(0, 20);

  oled.clearDisplay();
  delay(5);
  oled.clearBuffer();

  oled.setFontMode(1);
  oled.setFontDirection(0);
  oled.setDrawColor(1);
  oled.setFont(u8g2_font_5x8_tf);

  oled.drawStr(0, 10, line0.c_str());
  oled.drawStr(0, 21, line1.c_str());
  oled.drawStr(0, 32, envText.c_str());

  oled.drawLine(0, 36, 127, 36);

  if (lines[0].length() > 0) {
    if (lines[0].length() > 20) lines[0] = lines[0].substring(0, 20);
    oled.drawStr(0, 49, lines[0].c_str());
  }

  if (lines[1].length() > 0) {
    if (lines[1].length() > 20) lines[1] = lines[1].substring(0, 20);
    oled.drawStr(0, 60, lines[1].c_str());
  }

  oled.sendBuffer();
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
  COLOR_ALERT = M5.Display.color565(255, 75, 65);
  COLOR_ACTIVE = M5.Display.color565(110, 190, 255);
  COLOR_FLOWER = M5.Display.color565(255, 150, 210);
}

void drawSproutOnS3R() {
  sproutCanvas.fillScreen(COLOR_BG);
  sproutCanvas.setTextSize(1);
  sproutCanvas.setTextColor(COLOR_TEXT, COLOR_BG);

  sproutCanvas.setCursor(76, 4);
  sproutCanvas.print(placeToText(currentPlace));

  if (!isnan(lastTemperature)) {
    sproutCanvas.setCursor(76, 16);
    sproutCanvas.print(lastTemperature, 1);
    sproutCanvas.print("C");
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
  uint16_t accentColor = COLOR_TEXT;

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
    sproutCanvas.fillCircle(104, 22, 7, COLOR_SUN);
  } else if (currentState == STATE_WALKING) {
    sproutCanvas.drawLine(10, 48, 30, 48, COLOR_TEXT);
    sproutCanvas.drawLine(96, 58, 118, 58, COLOR_TEXT);
  }

  if (currentSound == SOUND_QUIET) {
    accentColor = COLOR_FLOWER;

    if (realtimePlant.calm > 65 && currentState != STATE_WILTED) {
      sproutCanvas.drawCircle(18, 42, 4, COLOR_FLOWER);
      sproutCanvas.fillCircle(18, 42, 2, COLOR_FLOWER);
    }
  } else if (currentSound == SOUND_ACTIVE) {
    accentColor = COLOR_ACTIVE;

    sproutCanvas.drawCircle(18, 42, 3, COLOR_ACTIVE);
    sproutCanvas.drawCircle(18, 42, 7, COLOR_ACTIVE);
    sproutCanvas.drawCircle(18, 42, 11, COLOR_ACTIVE);

    plantSway += (frame % 8 < 4) ? -2 : 2;
  } else if (currentSound == SOUND_INTENSE) {
    accentColor = COLOR_ALERT;

    int jitter = (frame % 2 == 0) ? -5 : 5;
    plantSway += jitter;
    topY += 8;
    leafDrop += 8;

    sproutCanvas.drawLine(12, 36, 24, 48, COLOR_ALERT);
    sproutCanvas.drawLine(24, 36, 12, 48, COLOR_ALERT);
  }

  if (realtimePlant.stress > 45) {
    sproutCanvas.drawCircle(centerX, topY + 18, 20, COLOR_ALERT);
    sproutCanvas.drawCircle(centerX, topY + 18, 24, COLOR_ALERT);
  }

  sproutCanvas.drawLine(centerX - 2, baseY, centerX + plantSway, topY, COLOR_STEM);
  sproutCanvas.drawLine(centerX - 1, baseY, centerX + plantSway + 1, topY, COLOR_STEM);
  sproutCanvas.drawLine(centerX, baseY, centerX + plantSway + 2, topY, COLOR_STEM);

  int activeLeafExtra = realtimePlant.curiosity > 55 ? 4 : 0;
  sproutCanvas.fillEllipse(centerX - 26 + plantSway, leafY + leafDrop, 24 + activeLeafExtra, 11, leafColor);
  sproutCanvas.fillEllipse(centerX + 26 + plantSway, leafY + leafDrop + 4, 24 + activeLeafExtra, 11, leafColor);
  sproutCanvas.fillEllipse(centerX + plantSway + 8,
                         topY + 4,
                         currentState == STATE_SUNLIGHT ? 12 : 9,
                         currentState == STATE_SUNLIGHT ? 6 : 5,
                         leafColor);

  if (realtimePlant.curiosity > 50 && currentState != STATE_WILTED) {
    sproutCanvas.drawLine(centerX + plantSway,
                        topY + 28,
                        centerX - 18 + plantSway,
                        topY + 16,
                        COLOR_STEM);

    sproutCanvas.fillEllipse(centerX - 25 + plantSway,
                           topY + 14,
                           12,
                           6,
                           leafColor);
  }

  if (realtimePlant.curiosity > 75 && currentState != STATE_WILTED) {
    sproutCanvas.drawLine(centerX + plantSway,
                        topY + 38,
                        centerX + 20 + plantSway,
                        topY + 24,
                        COLOR_STEM);

    sproutCanvas.fillEllipse(centerX + 28 + plantSway,
                           topY + 22,
                           12,
                           6,
                           leafColor);
  }

  if (realtimePlant.calm > 82 && currentState != STATE_WILTED) {
    sproutCanvas.fillCircle(centerX + plantSway, topY - 4, 4, COLOR_FLOWER);
    sproutCanvas.fillCircle(centerX + plantSway - 4, topY - 2, 3, COLOR_FLOWER);
    sproutCanvas.fillCircle(centerX + plantSway + 4, topY - 2, 3, COLOR_FLOWER);
    sproutCanvas.fillCircle(centerX + plantSway, topY + 1, 2, COLOR_SUN);
  }

  sproutCanvas.fillRoundRect(42, 112, 44, 10, 5, COLOR_POT);
  sproutCanvas.drawRoundRect(40, 110, 48, 14, 5, COLOR_TEXT);

  int calmW = map((int)realtimePlant.calm, 0, 100, 0, 38);
  int curiousW = map((int)realtimePlant.curiosity, 0, 100, 0, 38);
  int stressW = map((int)realtimePlant.stress, 0, 100, 0, 38);

  sproutCanvas.drawRect(4, 124, 38, 3, COLOR_TEXT);
  sproutCanvas.drawRect(45, 124, 38, 3, COLOR_ACTIVE);
  sproutCanvas.drawRect(86, 124, 38, 3, COLOR_ALERT);
  sproutCanvas.fillRect(4, 124, calmW, 3, COLOR_FLOWER);
  sproutCanvas.fillRect(45, 124, curiousW, 3, COLOR_ACTIVE);
  sproutCanvas.fillRect(86, 124, stressW, 3, COLOR_ALERT);

  sproutCanvas.pushSprite(0, 0);
}

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
  Serial.print(" mean:");
  Serial.print(lastSoundMean, 5);
  Serial.print(" peak:");
  Serial.print(lastSoundPeak, 5);
  Serial.print(" range:");
  Serial.print(lastSoundRange, 5);
  Serial.print(" var:");
  Serial.print(lastSoundVariance, 7);
  Serial.print(" calm:");
  Serial.print(realtimePlant.calm, 1);
  Serial.print(" curious:");
  Serial.print(realtimePlant.curiosity, 1);
  Serial.print(" stress:");
  Serial.print(realtimePlant.stress, 1);
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

void setup() {
  Serial.begin(115200);
  delay(1000);

  auto cfg = M5.config();
  cfg.external_speaker.atomic_echo = true;

  M5.begin(cfg);
  M5.Display.setBrightness(120);

  sproutCanvas.setColorDepth(16);
  sproutCanvas.createSprite(M5.Display.width(), M5.Display.height());

  initColors();

  Wire.begin(SDA_PIN, SCL_PIN);

  initBH1750();

  selectPaHubChannel(PAHUB_CHANNEL_OLED);
  oled.begin();
  oled.setBusClock(100000);
  oled.setContrast(180);
  oled.setFontMode(1);
  oled.setFontDirection(0);
  oled.setDrawColor(1);
  oled.clearDisplay();
  delay(50);
  oled.clearBuffer();
  oled.sendBuffer();
  delay(50);
  showTextOnOLED();
  Serial.println("OLED on PaHUB channel 1 initialized.");

  initEnvPro();
  initMic();
  connectWiFi();

  Serial.println("City Sprout PaHUB main started.");
}

void loop() {
  M5.update();
  handleSerialDemoCommand();

  unsigned long now = millis();

  updateEnvPro();

  if (now - lastSoundTime >= SOUND_INTERVAL_MS) {
    SoundState realSound = updateSoundState();
    currentSound = getEffectiveSoundState(realSound);
    updateRealtimePlantFromSound(currentSound);
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