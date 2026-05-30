#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <M5Unified.h>
#include <AudioOutput.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <U8g2lib.h>
#include <bme68xLibrary.h>
#include <math.h>
#include "arduino_secrets.h"

const char* WIFI_SSID = SECRET_WIFI_SSID;
const char* WIFI_PASSWORD = SECRET_WIFI_PASSWORD;
const char* SERVER_URL = SECRET_SERVER_URL;

#define SDA_PIN 2
#define SCL_PIN 1

#define PAHUB_ADDR 0x70

#define PAHUB_CHANNEL_DLIGHT 0
#define PAHUB_CHANNEL_OLED 1
#define PAHUB_CHANNEL_ENV 2

#define BH1750_ADDR 0x23

U8G2_SH1107_64X128_F_HW_I2C oled(
  U8G2_R0,
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
const unsigned long IMU_INTERVAL_MS = 40;
const unsigned long ENV_INTERVAL_MS = 3000;
const unsigned long DRAW_INTERVAL_MS = 35;
const unsigned long OLED_INTERVAL_MS = 5000;
const unsigned long SERVER_INTERVAL_MS = 20000;
const unsigned long PRINT_INTERVAL_MS = 1000;
const unsigned long TTS_WAIT_TIMEOUT_MS = 20000;
const unsigned long TTS_POLL_INTERVAL_MS = 500;
const int MP3_LOOPS_PER_TICK = 16;

static constexpr uint8_t M5_SPK_CHANNEL = 0;
static constexpr int PREALLOCATE_BUFFER_SIZE = 5 * 1024;
static constexpr int PREALLOCATE_CODEC_SIZE = 29192;

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
int micZeroPeakStreak = 0;

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

bool playbackBuffersReady = false;
bool pendingTtsPlay = false;
bool mp3Playing = false;
unsigned long ttsWaitStart = 0;
unsigned long lastTtsPollTime = 0;
char audioBaseUrl[96] = "";
char audioLibraryBaseUrl[128] = "";
char statusUrl[96] = "";
char audioUrlBuffer[192] = "";

static char oledCacheLine0[22] = "";
static char oledCacheLine1[22] = "";
static char oledCacheEnv[22] = "";
static char oledCacheSpeech0[22] = "";
static char oledCacheSpeech1[22] = "";
static bool oledCacheValid = false;

int frame = 0;

#define SCREEN_W 128
#define SCREEN_H 128

#define PLANT_GREEN  0xAFE5
#define DARK_GREEN   0x2589
#define FLOWER_WHITE 0xFFFF

#define EYE_CENTER  0
#define EYE_RIGHT   1
#define EYE_UP      2
#define EYE_DOWN    3
#define EYE_CLOSED  4
#define EYE_SQUEEZE 5

const float WALK_ENTER_LEVEL = 0.060;
const float WALK_EXIT_LEVEL = 0.035;
const float IMPULSE_TRIGGER_LEVEL = 0.24;
const float IMPULSE_TRIGGER_WHILE_WALKING = 0.46;
const float STRONG_IMPULSE_LEVEL = 0.42;

const unsigned long WALK_ENTER_MS = 1200;
const unsigned long WALK_EXIT_MS = 1300;
const unsigned long IMPULSE_SHAKE_DURATION_MS = 900;
const unsigned long IMPULSE_COOLDOWN_MS = 950;

const float TOP_BOUNCE = 0.82;
const float TOP_AIR_DRAG = 0.975;
const float TOP_RETURN_LERP = 0.028;
const float TOP_KICK_NORMAL = 2.6;
const float TOP_KICK_STRONG = 4.2;
const float TOP_MAX_SPEED = 4.8;

const unsigned long TOP_FREE_AFTER_KICK_MS = 2400;
const unsigned long TOP_FORCE_RETURN_MS = 6500;
const unsigned long TOP_NUDGE_INTERVAL_MS = 260;

float smoothLux = 100.0;
float plantLiftSmooth = 0.0;

float displayAccX = 0.0;
float displayAccY = 0.0;
float displayAccZ = 1.0;
float displayLastAccMag = 1.0;
float motionLevel = 0.0;

bool impulseShaking = false;
unsigned long impulseShakeStartTime = 0;
unsigned long impulseShakeUntil = 0;
unsigned long lastImpulseTriggerTime = 0;
float impulseAmplitude = 0.0;

bool walkingSway = false;
unsigned long walkMotionStartTime = 0;
unsigned long walkQuietStartTime = 0;
unsigned long walkingSwayStartTime = 0;
float walkSwayAmpSmooth = 0.0;

struct TopParticle {
  float x;
  float y;
  float vx;
  float vy;
  float homeX;
  float homeY;
  int r;
};

TopParticle topParticles[3];

bool topBurstActive = false;
bool topBurstReturning = false;
bool topBurstAsBloom = false;
unsigned long topBurstStartTime = 0;
unsigned long topBurstFreeUntil = 0;
unsigned long lastTopNudgeTime = 0;

unsigned long lastImuTime = 0;
unsigned long nextBlinkTime = 0;
unsigned long blinkUntil = 0;

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
uint16_t COLOR_DIM_GREEN;
uint16_t COLOR_FLOWER_PINK;
uint16_t COLOR_FLOWER_CENTER;

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

float luxToLift(float lux) {
  if (lux < 0) return 0.0;

  const float darkLux = 20.0;
  const float brightLux = 1800.0;

  float a = log10(lux + 1.0);
  float amin = log10(darkLux + 1.0);
  float amax = log10(brightLux + 1.0);
  float v = (a - amin) / (amax - amin);

  if (v < 0.0) v = 0.0;
  if (v > 1.0) v = 1.0;

  return -10.0 + v * 32.0;
}

void updateDisplayLightState() {
  if (lastLux >= 0) {
    smoothLux = smoothLux * 0.65 + lastLux * 0.35;
  }

  float targetLift = luxToLift(smoothLux);
  plantLiftSmooth = plantLiftSmooth * 0.60 + targetLift * 0.40;
}

bool isDisplaySunlit() {
  return smoothLux >= LUX_NEED_SUN_MAX;
}

void refreshTopHomes() {
  int lift = (int)round(plantLiftSmooth);

  int leftY = 50 - (lift * 3) / 4;
  int rightY = 50 - (lift * 3) / 4;
  int midY = 45 - lift;

  topParticles[0].homeX = 40;
  topParticles[0].homeY = leftY;
  topParticles[1].homeX = 64;
  topParticles[1].homeY = midY;
  topParticles[2].homeX = 88;
  topParticles[2].homeY = rightY;
}

void normalizeVector(float &x, float &y) {
  float len = sqrtf(x * x + y * y);

  if (len < 0.001) {
    x = 1.0;
    y = 0.0;
    return;
  }

  x /= len;
  y /= len;
}

void limitParticleSpeed(TopParticle &p, float maxSpeed) {
  float s = sqrtf(p.vx * p.vx + p.vy * p.vy);

  if (s > maxSpeed) {
    p.vx = p.vx / s * maxSpeed;
    p.vy = p.vy / s * maxSpeed;
  }
}

void kickTopParticles(float strength) {
  if (!topBurstActive) return;

  float k = strength * 2.6;
  if (k < 0.5) k = 0.5;
  if (k > 2.4) k = 2.4;

  for (int i = 0; i < 3; i++) {
    float dirX = random(-100, 101) / 100.0;
    float dirY = random(-100, 101) / 100.0;

    normalizeVector(dirX, dirY);
    topParticles[i].vx += dirX * k;
    topParticles[i].vy += dirY * k;

    limitParticleSpeed(topParticles[i], TOP_MAX_SPEED);
  }
}

void startTopBurst(float signal) {
  refreshTopHomes();

  topBurstAsBloom = isDisplaySunlit();

  topParticles[0].r = topBurstAsBloom ? 17 : 14;
  topParticles[1].r = topBurstAsBloom ? 19 : 16;
  topParticles[2].r = topBurstAsBloom ? 17 : 14;

  for (int i = 0; i < 3; i++) {
    topParticles[i].x = topParticles[i].homeX;
    topParticles[i].y = topParticles[i].homeY;
  }

  float kick = (signal > STRONG_IMPULSE_LEVEL) ? TOP_KICK_STRONG : TOP_KICK_NORMAL;

  topParticles[0].vx = -kick;
  topParticles[0].vy = -kick * 0.35;
  topParticles[1].vx = kick * 0.25;
  topParticles[1].vy = kick * 0.45;
  topParticles[2].vx = kick;
  topParticles[2].vy = -kick * 0.25;

  for (int i = 0; i < 3; i++) {
    limitParticleSpeed(topParticles[i], TOP_MAX_SPEED);
  }

  topBurstActive = true;
  topBurstReturning = false;
  topBurstStartTime = millis();
  topBurstFreeUntil = millis() + TOP_FREE_AFTER_KICK_MS;
  lastTopNudgeTime = 0;
}

void triggerImpulseShake(float signal) {
  impulseShaking = true;
  impulseShakeStartTime = millis();
  impulseShakeUntil = impulseShakeStartTime + IMPULSE_SHAKE_DURATION_MS;
  impulseAmplitude = (signal > STRONG_IMPULSE_LEVEL) ? 10.0 : 7.0;

  if (!topBurstActive) {
    startTopBurst(signal);
  } else {
    topBurstReturning = false;
    topBurstFreeUntil = millis() + TOP_FREE_AFTER_KICK_MS;
    kickTopParticles(signal);
  }
}

void updateWalkingSwayState(unsigned long now) {
  if (motionLevel > WALK_ENTER_LEVEL) {
    if (walkMotionStartTime == 0) {
      walkMotionStartTime = now;
    }

    walkQuietStartTime = 0;

    if (!walkingSway && now - walkMotionStartTime > WALK_ENTER_MS) {
      walkingSway = true;
      walkingSwayStartTime = now;
    }
  } else {
    walkMotionStartTime = 0;

    if (walkingSway && motionLevel < WALK_EXIT_LEVEL) {
      if (walkQuietStartTime == 0) {
        walkQuietStartTime = now;
      }

      if (now - walkQuietStartTime > WALK_EXIT_MS) {
        walkingSway = false;
        walkQuietStartTime = 0;
      }
    }
  }
}

void updateImpulseShakeState(unsigned long now, float impulseSignal) {
  float threshold = walkingSway ? IMPULSE_TRIGGER_WHILE_WALKING : IMPULSE_TRIGGER_LEVEL;

  if (impulseSignal > threshold &&
      now - lastImpulseTriggerTime > IMPULSE_COOLDOWN_MS) {
    triggerImpulseShake(impulseSignal);
    lastImpulseTriggerTime = now;
  }

  if (impulseShaking && now > impulseShakeUntil) {
    impulseShaking = false;
    impulseAmplitude = 0.0;
  }
}

void updateWalkSwayAmplitude() {
  float targetAmp = 0.0;

  if (walkingSway) {
    targetAmp = motionLevel > 0.12 ? 4.8 : 3.5;
  }

  walkSwayAmpSmooth = walkSwayAmpSmooth * 0.85 + targetAmp * 0.15;
}

void updateDisplayMotionEffects() {
  if (!M5.Imu.update()) {
    return;
  }

  auto imuData = M5.Imu.getImuData();
  displayAccX = imuData.accel.x;
  displayAccY = imuData.accel.y;
  displayAccZ = imuData.accel.z;

  float accMag = sqrtf(displayAccX * displayAccX + displayAccY * displayAccY + displayAccZ * displayAccZ);
  float motionByGravity = fabsf(accMag - 1.0);
  float jerk = fabsf(accMag - displayLastAccMag);
  displayLastAccMag = accMag;

  float impulseSignal = fmaxf(jerk * 1.8, motionByGravity);
  float motionRaw = fmaxf(motionByGravity, jerk * 1.2);
  motionLevel = motionLevel * 0.78 + motionRaw * 0.22;
  lastMotionStrength = motionLevel;

  unsigned long now = millis();
  updateWalkingSwayState(now);
  updateImpulseShakeState(now, impulseSignal);
  updateWalkSwayAmplitude();
}

void updateTopBurst() {
  if (!topBurstActive) return;

  refreshTopHomes();

  unsigned long now = millis();
  unsigned long elapsed = now - topBurstStartTime;

  if (impulseShaking && !topBurstReturning && now - lastTopNudgeTime > TOP_NUDGE_INTERVAL_MS) {
    kickTopParticles(motionLevel + 0.20);
    topBurstFreeUntil = now + TOP_FREE_AFTER_KICK_MS;
    lastTopNudgeTime = now;
  }

  if (!topBurstReturning && (now > topBurstFreeUntil || elapsed > TOP_FORCE_RETURN_MS)) {
    topBurstReturning = true;
  }

  if (topBurstReturning) {
    bool allHome = true;

    for (int i = 0; i < 3; i++) {
      TopParticle &p = topParticles[i];
      p.x = p.x + (p.homeX - p.x) * TOP_RETURN_LERP;
      p.y = p.y + (p.homeY - p.y) * TOP_RETURN_LERP;
      p.vx *= 0.78;
      p.vy *= 0.78;

      float dx = p.x - p.homeX;
      float dy = p.y - p.homeY;

      if (fabsf(dx) > 1.2 || fabsf(dy) > 1.2) {
        allHome = false;
      }
    }

    if (allHome) {
      topBurstActive = false;
      topBurstReturning = false;
    }

    return;
  }

  for (int i = 0; i < 3; i++) {
    TopParticle &p = topParticles[i];
    p.x += p.vx;
    p.y += p.vy;
    p.vx *= TOP_AIR_DRAG;
    p.vy *= TOP_AIR_DRAG;

    float minX = p.r;
    float maxX = SCREEN_W - p.r;
    float minY = p.r;
    float maxY = SCREEN_H - p.r;

    if (p.x < minX) {
      p.x = minX;
      p.vx = fabsf(p.vx) * TOP_BOUNCE;
    }

    if (p.x > maxX) {
      p.x = maxX;
      p.vx = -fabsf(p.vx) * TOP_BOUNCE;
    }

    if (p.y < minY) {
      p.y = minY;
      p.vy = fabsf(p.vy) * TOP_BOUNCE;
    }

    if (p.y > maxY) {
      p.y = maxY;
      p.vy = -fabsf(p.vy) * TOP_BOUNCE;
    }

    limitParticleSpeed(p, TOP_MAX_SPEED);
  }
}

int getImpulseShakeOffset() {
  if (!impulseShaking) return 0;

  unsigned long now = millis();
  float elapsed = (float)(now - impulseShakeStartTime);
  float total = (float)IMPULSE_SHAKE_DURATION_MS;

  if (elapsed < 0) elapsed = 0;
  if (elapsed > total) elapsed = total;

  float envelope = 1.0 - elapsed / total;
  float x = sinf(elapsed * 0.035) * impulseAmplitude * envelope;
  return (int)roundf(x);
}

int getWalkingSwayOffset() {
  if (walkSwayAmpSmooth < 0.3) return 0;

  unsigned long now = millis();
  float elapsed = (float)(now - walkingSwayStartTime);
  float x = sinf(elapsed * 0.0062) * walkSwayAmpSmooth;
  return (int)roundf(x);
}

int getTotalShakeOffset() {
  int total = getImpulseShakeOffset() + getWalkingSwayOffset();
  if (total > 12) total = 12;
  if (total < -12) total = -12;
  return total;
}

int chooseEyeState() {
  unsigned long now = millis();

  if (impulseShaking) return EYE_SQUEEZE;
  if (now < blinkUntil) return EYE_CLOSED;
  if (currentState == STATE_WILTED) return EYE_CLOSED;
  if (walkingSway || currentState == STATE_WALKING) return EYE_CENTER;
  if (isDisplaySunlit()) return EYE_UP;

  if (now > nextBlinkTime) {
    blinkUntil = now + 160;
    nextBlinkTime = now + random(5000, 12000);
    return EYE_CLOSED;
  }

  return EYE_CENTER;
}

void initMic() {
  M5.Speaker.end();
  delay(20);

  auto micCfg = M5.Mic.config();
  micCfg.sample_rate = SOUND_SAMPLE_RATE;
  micCfg.dma_buf_count = 3;
  micCfg.dma_buf_len = SOUND_SAMPLE_COUNT;
  micCfg.noise_filter_level = 0;
  micCfg.over_sampling = 1;
  micCfg.magnification = micCfg.use_adc ? 16 : 1;
  M5.Mic.config(micCfg);

  micReady = M5.Mic.begin();
  delay(80);

  Serial.print("M5.Mic.begin(): ");
  Serial.println(micReady ? "true" : "false");
  Serial.print("M5.Mic.isEnabled(): ");
  Serial.println(M5.Mic.isEnabled() ? "true" : "false");

  micZeroPeakStreak = 0;

  if (!micReady) {
    Serial.println("WARN: Mic init failed.");
    return;
  }

  bool warmedUp = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    if (!M5.Mic.record(micBuffer, SOUND_SAMPLE_COUNT, SOUND_SAMPLE_RATE)) {
      delay(40);
      continue;
    }

    calculateSoundFeaturesFromBuffer();
    Serial.print("Mic warmup attempt ");
    Serial.print(attempt + 1);
    Serial.print(" level=");
    Serial.print(lastSoundLevel, 6);
    Serial.print(" peak=");
    Serial.println(lastSoundPeak, 6);

    if (lastSoundPeak > 0.0001) {
      warmedUp = true;
      break;
    }

    delay(40);
  }

  if (!warmedUp) {
    Serial.println("WARN: Mic reads silence. Clap near Voice Base or re-seat the stack.");
  }
}

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

void buildServerUrls() {
  String base = String(SERVER_URL);
  int slashIndex = base.lastIndexOf('/');

  if (slashIndex > 0) {
    base = base.substring(0, slashIndex);
  }

  snprintf(audioBaseUrl, sizeof(audioBaseUrl), "%s/audio/latest.mp3", base.c_str());
  snprintf(audioLibraryBaseUrl, sizeof(audioLibraryBaseUrl), "%s/audio/library", base.c_str());
  snprintf(statusUrl, sizeof(statusUrl), "%s/latest", base.c_str());
}

bool initPlaybackBuffers() {
  if (playbackBuffersReady) return true;

  preallocateBuffer = malloc(PREALLOCATE_BUFFER_SIZE);
  preallocateCodec = malloc(PREALLOCATE_CODEC_SIZE);

  if (preallocateBuffer == nullptr || preallocateCodec == nullptr) {
    Serial.println("Failed to allocate MP3 buffers.");
    return false;
  }

  buildServerUrls();
  playbackBuffersReady = true;
  Serial.print("TTS audio URL: ");
  Serial.println(audioBaseUrl);
  Serial.print("Audio library URL: ");
  Serial.println(audioLibraryBaseUrl);
  return true;
}

void initSpeakerForPlayback() {
  M5.Mic.end();
  micReady = false;
  delay(40);

  auto spkCfg = M5.Speaker.config();
  spkCfg.sample_rate = 64000;
  M5.Speaker.config(spkCfg);

  if (!M5.Speaker.isEnabled()) {
    M5.Speaker.begin();
  }

  M5.Speaker.setVolume(180);
  Serial.println("Speaker ready for TTS playback.");
}

void stopMp3Playback() {
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

void shutdownSpeakerRestoreMic() {
  stopMp3Playback();
  M5.Speaker.end();
  delay(120);
  initMic();
  Serial.println("Mic restored after TTS playback.");
}

bool startMp3Playback(const char* url) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot play: WiFi is not connected.");
    return false;
  }

  stopMp3Playback();

  Serial.print("Opening MP3: ");
  Serial.println(url);

  httpFile = new AudioFileSourceHTTPStream(url);
  bufferedFile = new AudioFileSourceBuffer(httpFile, preallocateBuffer, PREALLOCATE_BUFFER_SIZE);
  mp3 = new AudioGeneratorMP3(preallocateCodec, PREALLOCATE_CODEC_SIZE);

  bool ok = mp3->begin(bufferedFile, &audioOut);
  Serial.println(ok ? "MP3 playback started." : "MP3 playback failed to start.");
  return ok;
}

const char* audioPrefixForState(PlantState state) {
  if (state == STATE_WILTED) return "wilted";
  if (state == STATE_NEED_SUN) return "need_sun";
  if (state == STATE_SUNLIGHT) return "sunlight";
  if (state == STATE_WALKING) return "walking";
  return "idle";
}

bool playRandomStateAudio(PlantState state) {
  if (!playbackBuffersReady || WiFi.status() != WL_CONNECTED) return false;
  if (mp3Playing || pendingTtsPlay) return false;

  const char* prefix = audioPrefixForState(state);
  int index = random(1, 11);

  snprintf(
    audioUrlBuffer,
    sizeof(audioUrlBuffer),
    "%s/%s_%02d.mp3?t=%lu",
    audioLibraryBaseUrl,
    prefix,
    index,
    millis()
  );

  initSpeakerForPlayback();

  if (startMp3Playback(audioUrlBuffer)) {
    mp3Playing = true;
    return true;
  }

  shutdownSpeakerRestoreMic();
  return false;
}

bool isTtsReady() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  WiFiClient client;
  http.begin(client, statusUrl);
  int httpCode = http.GET();

  if (httpCode != 200) {
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();
  return body.indexOf("\"tts_status\":\"ready\"") >= 0;
}

void invalidateOledCache() {
  oledCacheValid = false;
  oledCacheLine0[0] = '\0';
  oledCacheLine1[0] = '\0';
  oledCacheEnv[0] = '\0';
  oledCacheSpeech0[0] = '\0';
  oledCacheSpeech1[0] = '\0';
}

bool isUiPausedForAudio() {
  return mp3Playing || pendingTtsPlay;
}

bool serviceMp3Playback() {
  if (mp3 == nullptr || !mp3->isRunning()) {
    return false;
  }

  for (int i = 0; i < MP3_LOOPS_PER_TICK; i++) {
    if (!mp3->loop()) {
      Serial.println("MP3 playback finished.");
      mp3Playing = false;
      shutdownSpeakerRestoreMic();
      invalidateOledCache();
      return false;
    }
  }

  return true;
}

void scheduleTtsPlayback() {
  if (!playbackBuffersReady || WiFi.status() != WL_CONNECTED) return;

  pendingTtsPlay = true;
  ttsWaitStart = millis();
  lastTtsPollTime = 0;
  Serial.println("Waiting for server TTS...");
}

void handleTtsPlayback(unsigned long now) {
  if (mp3 != nullptr && !mp3->isRunning()) {
    mp3Playing = false;
    shutdownSpeakerRestoreMic();
    invalidateOledCache();
  }

  if (!pendingTtsPlay) return;

  if (now - ttsWaitStart > TTS_WAIT_TIMEOUT_MS) {
    Serial.println("TTS wait timeout.");
    pendingTtsPlay = false;
    return;
  }

  if (now - lastTtsPollTime < TTS_POLL_INTERVAL_MS) return;
  lastTtsPollTime = now;

  if (!isTtsReady()) return;

  pendingTtsPlay = false;
  initSpeakerForPlayback();

  snprintf(audioUrlBuffer, sizeof(audioUrlBuffer), "%s?t=%lu", audioBaseUrl, now);
  if (startMp3Playback(audioUrlBuffer)) {
    mp3Playing = true;
  } else {
    shutdownSpeakerRestoreMic();
  }
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

  if (lastSoundPeak < 0.0001) {
    micZeroPeakStreak++;
    if (micZeroPeakStreak >= 24) {
      Serial.println("Mic stuck at zero, re-init...");
      initMic();
    }
  } else {
    micZeroPeakStreak = 0;
  }

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

String askPlantServer(bool* serverOk) {
  if (serverOk != nullptr) {
    *serverOk = false;
  }

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
    if (serverOk != nullptr) {
      *serverOk = true;
    }

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
  const int MAX_CHARS_PER_LINE = 10;

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

String fitOLEDText(String text, uint8_t maxWidth) {
  text.trim();

  while (text.length() > 0 && oled.getStrWidth(text.c_str()) > maxWidth) {
    text.remove(text.length() - 1);
  }

  return text;
}

void drawCenteredOLEDText(int y, const char* text) {
  int w = oled.getStrWidth(text);
  int x = (64 - w) / 2;
  if (x < 0) x = 0;
  oled.drawStr(x, y, text);
}

void drawCenteredSmallOLEDText(int y, const char* text) {
  oled.setFont(u8g2_font_5x8_tf);
  drawCenteredOLEDText(y, text);
  oled.setFont(u8g2_font_6x10_tf);
}

String compactOLEDStateTitle(PlantState state) {
  if (state == STATE_WILTED) return "WILTED";
  if (state == STATE_NEED_SUN) return "NEED SUN";
  if (state == STATE_SUNLIGHT) return "SUN";
  if (state == STATE_WALKING) return "WALKING";
  return "READY";
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

void showTextOnOLED(bool forceUpdate = false) {
  selectPaHubChannel(PAHUB_CHANNEL_OLED);

  String speechSafe = toOLEDAscii(lastSpeech);
  String speechLines[2];
  wrapTextForOLED(speechSafe, speechLines, 2);

  String line0 = "SPROUT";
  String line1 = compactOLEDStateTitle(currentState);
  String envText = "";

  oled.setFont(u8g2_font_6x10_tf);
  line0 = fitOLEDText(line0, 46);
  line1 = fitOLEDText(line1, 58);

  String speech0 = speechLines[0];
  String speech1 = speechLines[1];
  oled.setFont(u8g2_font_5x8_tf);
  speech0 = fitOLEDText(speech0, 48);
  speech1 = fitOLEDText(speech1, 48);

  if (!forceUpdate && oledCacheValid &&
      line0 == oledCacheLine0 &&
      line1 == oledCacheLine1 &&
      envText == oledCacheEnv &&
      speech0 == oledCacheSpeech0 &&
      speech1 == oledCacheSpeech1) {
    return;
  }

  oled.clearBuffer();

  oled.setFontMode(1);
  oled.setFontDirection(0);
  oled.setDrawColor(1);

  oled.setFont(u8g2_font_6x10_tf);

  oled.drawRFrame(1, 1, 62, 126, 6);
  oled.drawRBox(6, 6, 52, 14, 4);

  oled.setDrawColor(0);
  drawCenteredOLEDText(16, line0.c_str());
  oled.setDrawColor(1);

  oled.drawDisc(10, 27, 1);
  oled.drawDisc(16, 27, 1);
  oled.drawLine(22, 27, 54, 27);

  drawCenteredOLEDText(39, line1.c_str());

  oled.drawRFrame(6, 50, 52, 60, 5);
  drawCenteredSmallOLEDText(76, speech0.c_str());
  drawCenteredSmallOLEDText(94, speech1.c_str());

  oled.drawLine(10, 118, 54, 118);
  oled.drawDisc(18, 122, 1);
  oled.drawDisc(32, 122, 1);
  oled.drawDisc(46, 122, 1);

  oled.sendBuffer();

  line0.toCharArray(oledCacheLine0, sizeof(oledCacheLine0));
  line1.toCharArray(oledCacheLine1, sizeof(oledCacheLine1));
  envText.toCharArray(oledCacheEnv, sizeof(oledCacheEnv));
  speech0.toCharArray(oledCacheSpeech0, sizeof(oledCacheSpeech0));
  speech1.toCharArray(oledCacheSpeech1, sizeof(oledCacheSpeech1));
  oledCacheValid = true;
}

void initColors() {
  COLOR_BG = M5.Display.color565(12, 16, 14);
  COLOR_STEM = M5.Display.color565(210, 230, 200);
  COLOR_LEAF = PLANT_GREEN;
  COLOR_LEAF_LIGHT = M5.Display.color565(110, 220, 95);
  COLOR_LEAF_DARK = M5.Display.color565(48, 85, 55);
  COLOR_SUN = M5.Display.color565(255, 205, 80);
  COLOR_TEXT = M5.Display.color565(230, 240, 220);
  COLOR_POT = M5.Display.color565(90, 60, 40);
  COLOR_ALERT = M5.Display.color565(255, 75, 65);
  COLOR_ACTIVE = M5.Display.color565(110, 190, 255);
  COLOR_FLOWER = M5.Display.color565(255, 150, 210);
  COLOR_DIM_GREEN = M5.Display.color565(55, 105, 58);
  COLOR_FLOWER_PINK = M5.Display.color565(238, 176, 196);
  COLOR_FLOWER_CENTER = M5.Display.color565(245, 220, 150);
}

void drawThickLine(int x1, int y1, int x2, int y2, uint16_t color) {
  sproutCanvas.drawLine(x1, y1, x2, y2, color);
  sproutCanvas.drawLine(x1 + 1, y1, x2 + 1, y2, color);
  sproutCanvas.drawLine(x1, y1 + 1, x2, y2 + 1, color);
}

void drawGreaterEye(int cx, int cy) {
  drawThickLine(cx - 5, cy - 6, cx + 5, cy, DARK_GREEN);
  drawThickLine(cx + 5, cy, cx - 5, cy + 6, DARK_GREEN);
}

void drawLessEye(int cx, int cy) {
  drawThickLine(cx + 5, cy - 6, cx - 5, cy, DARK_GREEN);
  drawThickLine(cx - 5, cy, cx + 5, cy + 6, DARK_GREEN);
}

void drawPinkFlower(int cx, int cy, int baseR) {
  int petalR = baseR * 50 / 100;
  int spread = baseR * 52 / 100;

  if (petalR < 6) petalR = 6;
  if (spread < 7) spread = 7;

  sproutCanvas.fillCircle(cx, cy - spread, petalR, COLOR_FLOWER_PINK);
  sproutCanvas.fillCircle(cx - spread, cy - 1, petalR, COLOR_FLOWER_PINK);
  sproutCanvas.fillCircle(cx + spread, cy - 1, petalR, COLOR_FLOWER_PINK);
  sproutCanvas.fillCircle(cx - spread / 2, cy + spread - 1, petalR, COLOR_FLOWER_PINK);
  sproutCanvas.fillCircle(cx + spread / 2, cy + spread - 1, petalR, COLOR_FLOWER_PINK);

  int centerR = baseR * 24 / 100;
  if (centerR < 3) centerR = 3;

  sproutCanvas.fillCircle(cx, cy, centerR, COLOR_FLOWER_CENTER);
}

void drawFlyingTopObjects() {
  if (!topBurstActive) return;

  for (int i = 0; i < 3; i++) {
    int x = (int)round(topParticles[i].x);
    int y = (int)round(topParticles[i].y);

    if (topBurstAsBloom) {
      drawPinkFlower(x, y, topParticles[i].r);
    } else {
      sproutCanvas.fillCircle(x, y, topParticles[i].r, FLOWER_WHITE);
    }
  }
}

void drawSproutOnS3R() {
  sproutCanvas.fillScreen(COLOR_BG);

  int lift = (int)round(plantLiftSmooth);
  int shakeX = getTotalShakeOffset();
  int eyeState = chooseEyeState();

  int y1 = 60 - lift;
  int y2 = 72 - (lift * 2) / 3;
  int y3 = 84 - (lift * 1) / 3;
  int y4 = 96;

  int w1 = 80 + lift / 3;
  int w2 = 96 + lift / 4;
  int w3 = 88 + lift / 5;
  int w4 = 72;

  if (w1 < 70) w1 = 70;
  if (w2 < 86) w2 = 86;
  if (w3 < 80) w3 = 80;
  if (w1 > 90) w1 = 90;
  if (w2 > 104) w2 = 104;
  if (w3 > 96) w3 = 96;

  int x1 = 64 - w1 / 2 + shakeX / 2;
  int x2 = 64 - w2 / 2 + shakeX / 3;
  int x3 = 64 - w3 / 2 + shakeX / 5;
  int x4 = 64 - w4 / 2;

  uint16_t plantColor = (currentState == STATE_WILTED) ? COLOR_DIM_GREEN : PLANT_GREEN;

  if (currentSound == SOUND_INTENSE) {
    plantColor = COLOR_DIM_GREEN;
  }

  sproutCanvas.fillRoundRect(x1, y1, w1, 16, 4, plantColor);
  sproutCanvas.fillRoundRect(x2, y2, w2, 16, 4, plantColor);
  sproutCanvas.fillRoundRect(x3, y3, w3, 16, 4, plantColor);
  sproutCanvas.fillRoundRect(x4, y4, w4, 12, 4, plantColor);

  int leftFlowerY = 50 - (lift * 3) / 4;
  int rightFlowerY = 50 - (lift * 3) / 4;
  int midFlowerY = 45 - lift;

  if (topBurstActive) {
    drawFlyingTopObjects();
  } else if (isDisplaySunlit()) {
    drawPinkFlower(40 + shakeX, leftFlowerY, 17);
    drawPinkFlower(88 + shakeX, rightFlowerY, 17);
    drawPinkFlower(64 + shakeX, midFlowerY, 19);
    sproutCanvas.fillCircle(112, 18, 3, COLOR_SUN);
    sproutCanvas.fillCircle(105, 25, 1, COLOR_SUN);
  } else {
    sproutCanvas.fillCircle(40 + shakeX, leftFlowerY, 14, FLOWER_WHITE);
    sproutCanvas.fillCircle(88 + shakeX, rightFlowerY, 14, FLOWER_WHITE);
    sproutCanvas.fillCircle(64 + shakeX, midFlowerY, 16, FLOWER_WHITE);
  }

  if (currentSound == SOUND_ACTIVE) {
    sproutCanvas.drawCircle(18, 42, 3, COLOR_ACTIVE);
    sproutCanvas.drawCircle(18, 42, 7, COLOR_ACTIVE);
    sproutCanvas.drawCircle(18, 42, 11, COLOR_ACTIVE);
  } else if (currentSound == SOUND_INTENSE) {
    sproutCanvas.drawLine(12, 36, 24, 48, COLOR_ALERT);
    sproutCanvas.drawLine(24, 36, 12, 48, COLOR_ALERT);
  } else if (currentSound == SOUND_QUIET && realtimePlant.calm > 65) {
    sproutCanvas.drawCircle(18, 42, 4, COLOR_FLOWER);
    sproutCanvas.fillCircle(18, 42, 2, COLOR_FLOWER);
  }

  int eyeY = 80 - lift / 4;
  int eyeShake = shakeX / 3;

  if (eyeState == EYE_SQUEEZE) {
    drawGreaterEye(50 + eyeShake, eyeY);
    drawLessEye(78 + eyeShake, eyeY);
  } else if (eyeState == EYE_CLOSED) {
    sproutCanvas.fillRect(44 + eyeShake, eyeY, 12, 4, DARK_GREEN);
    sproutCanvas.fillRect(72 + eyeShake, eyeY, 12, 4, DARK_GREEN);
  } else {
    sproutCanvas.fillCircle(50 + eyeShake, eyeY, 8, FLOWER_WHITE);
    sproutCanvas.fillCircle(78 + eyeShake, eyeY, 8, FLOWER_WHITE);

    int offsetX = 0;
    int offsetY = 0;

    if (eyeState == EYE_RIGHT) offsetX = 3;
    if (eyeState == EYE_UP) offsetY = -3;
    if (eyeState == EYE_DOWN) offsetY = 3;

    sproutCanvas.fillCircle(50 + eyeShake + offsetX, eyeY + offsetY, 4, DARK_GREEN);
    sproutCanvas.fillCircle(78 + eyeShake + offsetX, eyeY + offsetY, 4, DARK_GREEN);
  }

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
  /*
    麦克风稳定工作只需要 atomic_echo。
    旧版同时打开 atomic_spk 时，Voice Base 可能出现 Mic enabled 但读数全 0。
    TTS 播放仍可通过 atomic_echo 驱动外接扬声器。
  */
  cfg.external_speaker.atomic_echo = true;

  M5.begin(cfg);
  M5.Display.setRotation(3);
  M5.Display.setBrightness(120);

  /*
    麦克风必须在 Canvas 大内存分配之前初始化。
    否则 Voice Base 可能出现 Mic enabled 但 level/peak 永远为 0。
  */
  initMic();

  sproutCanvas.setColorDepth(16);
  sproutCanvas.createSprite(M5.Display.width(), M5.Display.height());

  initColors();
  randomSeed(millis());
  nextBlinkTime = millis() + random(5000, 12000);

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
  showTextOnOLED(true);
  Serial.println("OLED on PaHUB channel 1 initialized.");

  initEnvPro();
  connectWiFi();
  initPlaybackBuffers();

  Serial.println("City Sprout PaHUB main started.");
}

void loop() {
  M5.update();
  handleSerialDemoCommand();

  unsigned long now = millis();

  handleTtsPlayback(now);

  if (serviceMp3Playback()) {
    return;
  }

  if (!mp3Playing) {
    updateEnvPro();
  }

  if (!isUiPausedForAudio() && now - lastImuTime >= IMU_INTERVAL_MS) {
    updateDisplayMotionEffects();
    lastImuTime = now;
  }

  if (!isUiPausedForAudio()) {
    updateTopBurst();
  }

  if (!isUiPausedForAudio() && now - lastSoundTime >= SOUND_INTERVAL_MS) {
    SoundState realSound = updateSoundState();
    currentSound = getEffectiveSoundState(realSound);
    updateRealtimePlantFromSound(currentSound);
    lastSoundTime = now;
  }

  if (!isUiPausedForAudio() && now - lastSensorTime >= SENSOR_INTERVAL_MS) {
    lastLux = readLightLux();
    updateDisplayLightState();

    currentMotion = updateMotionState(lastMotionStrength);

    currentState = getStateFromLux(smoothLux);
    if (currentMotion == MOTION_ACTIVE || walkingSway || impulseShaking) {
      currentState = STATE_WALKING;
    }

    currentPlace = updatePlaceState(lastLux, currentMotion, currentSound);
    lastSensorTime = now;
  }

  if (!isUiPausedForAudio() && now - lastDrawTime >= DRAW_INTERVAL_MS) {
    drawSproutOnS3R();
    frame++;
    lastDrawTime = now;
  }

  if (!isUiPausedForAudio() && now - lastOledTime >= OLED_INTERVAL_MS) {
    showTextOnOLED(false);
    lastOledTime = now;
  }

  if (lastLux >= 0 && !mp3Playing && now - lastServerTime >= SERVER_INTERVAL_MS) {
    bool serverOk = false;
    lastSpeech = askPlantServer(&serverOk);
    Serial.print("Plant says: ");
    Serial.println(lastSpeech);
    invalidateOledCache();
    showTextOnOLED(true);

    if (!playRandomStateAudio(currentState) && serverOk) {
      scheduleTtsPlayback();
    }

    lastServerTime = now;
  }

  if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
    printDebugLine();
    lastPrintTime = now;
  }
}
