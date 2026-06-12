#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <math.h>

// ======================================================
// City Sprout 感光 + 移动区分 + 慢速玻璃球飞散版
//
// 功能：
// 1. DLight / BH1750 控制植物高矮
// 2. 短时间拿起/摇晃：快速大幅摇摆 + 表情变成 >  <
// 3. 短时间摇晃时：头顶白球 / 粉花像瓶子里的玻璃球一样在屏幕内慢速飞散
// 4. 小球碰到屏幕边界会轻微回弹
// 5. 继续摇晃时，小球会被轻轻重新搅动
// 6. 停止摇晃后，小球继续滑动一小段时间，再缓慢回到头顶
// 7. 长时间走路 / 包挂晃动：慢速持续摇摆，不触发玻璃球乱飞
// 8. 光线充分时：头顶三个白点变成三朵低饱和粉色小花
// ======================================================


// ==================== I2C / PaHUB / DLight ====================
#define SDA_PIN 2
#define SCL_PIN 1

#define PAHUB_ADDR 0x70
#define PAHUB_CHANNEL_DLIGHT 0

#define BH1750_ADDR 0x23


// ==================== 屏幕尺寸 ====================
#define SCREEN_W 128
#define SCREEN_H 128


// ==================== 光照状态阈值 ====================
const float LUX_WILTED_MAX = 50.0;
const float LUX_NEED_SUN_MAX = 300.0;
const float LUX_OUTSIDE_HINT = 800.0;


// ==================== 移动检测参数 ====================
const float WALK_ENTER_LEVEL = 0.060;
const float WALK_EXIT_LEVEL  = 0.035;

const float IMPULSE_TRIGGER_LEVEL = 0.24;
const float IMPULSE_TRIGGER_WHILE_WALKING = 0.46;
const float STRONG_IMPULSE_LEVEL = 0.42;

const unsigned long WALK_ENTER_MS = 1200;
const unsigned long WALK_EXIT_MS = 1300;

const unsigned long IMPULSE_SHAKE_DURATION_MS = 900;
const unsigned long IMPULSE_COOLDOWN_MS = 950;


// ==================== 顶部白球 / 花朵飞散参数 ====================
// 慢速玻璃球效果：
// - 无重力
// - 速度更慢
// - 碰壁轻微回弹
// - 只因为摇晃获得速度
// - 停止后慢慢回到头顶

const float TOP_BOUNCE = 0.82;
const float TOP_AIR_DRAG = 0.975;
const float TOP_RETURN_LERP = 0.028;

const float TOP_KICK_NORMAL = 2.6;
const float TOP_KICK_STRONG = 4.2;
const float TOP_MAX_SPEED = 4.8;

const unsigned long TOP_FREE_AFTER_KICK_MS = 2400;
const unsigned long TOP_FORCE_RETURN_MS = 6500;
const unsigned long TOP_NUDGE_INTERVAL_MS = 260;


// ==================== 显示设置 ====================
M5Canvas sproutCanvas(&M5.Display);

const unsigned long SENSOR_INTERVAL_MS = 150;
const unsigned long IMU_INTERVAL_MS = 40;
const unsigned long DRAW_INTERVAL_MS = 35;
const unsigned long PRINT_INTERVAL_MS = 1000;

// 调试阶段建议 1；正式展示可以改成 0
#define SHOW_DEBUG_TEXT 1


// ==================== 颜色 ====================
#define PLANT_GREEN  0xAFE5
#define DARK_GREEN   0x2589
#define FLOWER_WHITE 0xFFFF

uint16_t COLOR_BG;
uint16_t COLOR_TEXT;
uint16_t COLOR_SUN;
uint16_t COLOR_DIM_GREEN;
uint16_t COLOR_FLOWER_PINK;
uint16_t COLOR_FLOWER_CENTER;


// ==================== 植物状态 ====================
enum PlantState {
  STATE_IDLE,
  STATE_WILTED,
  STATE_NEED_SUN,
  STATE_SUNLIGHT
};

PlantState currentState = STATE_IDLE;


// ==================== 眼睛状态 ====================
#define EYE_CENTER  0
#define EYE_RIGHT   1
#define EYE_UP      2
#define EYE_DOWN    3
#define EYE_CLOSED  4
#define EYE_SQUEEZE 5


// ==================== 光照变量 ====================
bool dlightReady = false;

float lastLux = -1.0;
float smoothLux = 100.0;
float plantLiftSmooth = 0.0;


// ==================== IMU / 移动变量 ====================
bool imuReady = false;

float accX = 0.0;
float accY = 0.0;
float accZ = 1.0;

float lastAccMag = 1.0;
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


// ==================== 顶部白球 / 花朵飞散变量 ====================
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


// ==================== 时间变量 ====================
unsigned long lastSensorTime = 0;
unsigned long lastImuTime = 0;
unsigned long lastDrawTime = 0;
unsigned long lastPrintTime = 0;

unsigned long nextBlinkTime = 0;
unsigned long blinkUntil = 0;

int frame = 0;


// ======================================================
// 函数声明
// ======================================================

void updateWalkingSwayState(unsigned long now);
void updateImpulseShakeState(unsigned long now, float impulseSignal);
void updateWalkSwayAmplitude();

void triggerImpulseShake(float signal);

void startTopBurst(float signal);
void kickTopParticles(float strength);
void updateTopBurst();


// ======================================================
// PaHUB / BH1750
// ======================================================

bool selectPaHubChannel(uint8_t channel) {
  if (channel > 5) return false;

  Wire.beginTransmission(PAHUB_ADDR);
  Wire.write(1 << channel);
  byte error = Wire.endTransmission();

  delayMicroseconds(500);
  return error == 0;
}

bool i2cDeviceExists(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void writeBH1750Command(byte command) {
  selectPaHubChannel(PAHUB_CHANNEL_DLIGHT);
  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(command);
  Wire.endTransmission();
}

bool initBH1750() {
  selectPaHubChannel(PAHUB_CHANNEL_DLIGHT);
  delay(20);

  if (!i2cDeviceExists(BH1750_ADDR)) {
    Serial.println("DLight / BH1750 not found on PaHUB channel 0.");
    return false;
  }

  writeBH1750Command(0x01);
  delay(10);

  writeBH1750Command(0x07);
  delay(10);

  writeBH1750Command(0x10);
  delay(200);

  Serial.println("DLight on PaHUB channel 0 initialized.");
  return true;
}

float readLightLux() {
  if (!dlightReady) return -1.0;

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


// ======================================================
// 光照状态
// ======================================================

PlantState getStateFromLux(float lux) {
  if (lux < 0) return STATE_IDLE;
  if (lux < LUX_WILTED_MAX) return STATE_WILTED;
  if (lux < LUX_NEED_SUN_MAX) return STATE_NEED_SUN;
  return STATE_SUNLIGHT;
}

const char* plantStateToText(PlantState state) {
  switch (state) {
    case STATE_WILTED:    return "DIM";
    case STATE_NEED_SUN:  return "NEED";
    case STATE_SUNLIGHT:  return "SUN";
    case STATE_IDLE:
    default:              return "IDLE";
  }
}

float luxToLift(float lux) {
  if (lux < 0) return 0.0;

  float darkLux = LUX_WILTED_MAX;
  float brightLux = LUX_OUTSIDE_HINT;

  if (brightLux < 1000.0) {
    brightLux = 1000.0;
  }

  float a = log10(lux + 1.0);
  float amin = log10(darkLux + 1.0);
  float amax = log10(brightLux + 1.0);

  float v = (a - amin) / (amax - amin);

  if (v < 0.0) v = 0.0;
  if (v > 1.0) v = 1.0;

  return -10.0 + v * 32.0;
}

void updateLightState() {
  float newLux = readLightLux();

  if (newLux >= 0) {
    lastLux = newLux;
    smoothLux = smoothLux * 0.65 + lastLux * 0.35;
  }

  currentState = getStateFromLux(smoothLux);

  float targetLift = luxToLift(smoothLux);
  plantLiftSmooth = plantLiftSmooth * 0.60 + targetLift * 0.40;
}


// ======================================================
// IMU 移动检测
// ======================================================

void updateMotionState() {
  if (!M5.Imu.update()) {
    return;
  }

  imuReady = true;

  auto imuData = M5.Imu.getImuData();

  accX = imuData.accel.x;
  accY = imuData.accel.y;
  accZ = imuData.accel.z;

  float accMag = sqrtf(accX * accX + accY * accY + accZ * accZ);

  float motionByGravity = fabsf(accMag - 1.0);
  float jerk = fabsf(accMag - lastAccMag);
  lastAccMag = accMag;

  float impulseSignal = fmaxf(jerk * 1.8, motionByGravity);
  float motionRaw = fmaxf(motionByGravity, jerk * 1.2);

  motionLevel = motionLevel * 0.78 + motionRaw * 0.22;

  unsigned long now = millis();

  updateWalkingSwayState(now);
  updateImpulseShakeState(now, impulseSignal);
  updateWalkSwayAmplitude();
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
      Serial.println("Walking sway ON.");
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
        Serial.println("Walking sway OFF.");
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

void triggerImpulseShake(float signal) {
  impulseShaking = true;
  impulseShakeStartTime = millis();
  impulseShakeUntil = impulseShakeStartTime + IMPULSE_SHAKE_DURATION_MS;

  if (signal > STRONG_IMPULSE_LEVEL) {
    impulseAmplitude = 10.0;
  } else {
    impulseAmplitude = 7.0;
  }

  // 第一次短摇：启动飞散
  // 已经飞散时再次短摇：轻轻搅动
  if (!topBurstActive) {
    startTopBurst(signal);
  } else {
    topBurstReturning = false;
    topBurstFreeUntil = millis() + TOP_FREE_AFTER_KICK_MS;
    kickTopParticles(signal);
  }

  Serial.print("Impulse shake triggered. signal=");
  Serial.println(signal);
}

void updateWalkSwayAmplitude() {
  float targetAmp = 0.0;

  if (walkingSway) {
    targetAmp = 3.5;

    if (motionLevel > 0.12) {
      targetAmp = 4.8;
    }
  }

  walkSwayAmpSmooth = walkSwayAmpSmooth * 0.85 + targetAmp * 0.15;
}


// ======================================================
// 顶部白球 / 花朵飞散逻辑：慢速无重力玻璃球版
// ======================================================

void refreshTopHomes() {
  int lift = (int)round(plantLiftSmooth);

  int leftY  = 50 - (lift * 3) / 4;
  int rightY = 50 - (lift * 3) / 4;
  int midY   = 45 - lift;

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

void startTopBurst(float signal) {
  refreshTopHomes();

  topBurstAsBloom = (currentState == STATE_SUNLIGHT);

  if (topBurstAsBloom) {
    topParticles[0].r = 17;
    topParticles[1].r = 19;
    topParticles[2].r = 17;
  } else {
    topParticles[0].r = 14;
    topParticles[1].r = 16;
    topParticles[2].r = 14;
  }

  for (int i = 0; i < 3; i++) {
    topParticles[i].x = topParticles[i].homeX;
    topParticles[i].y = topParticles[i].homeY;
  }

  float kick = (signal > STRONG_IMPULSE_LEVEL) ? TOP_KICK_STRONG : TOP_KICK_NORMAL;

  // 慢速四散，像瓶子里的小球被晃开
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

void kickTopParticles(float strength) {
  if (!topBurstActive) return;

  // 轻轻搅动，而不是爆炸式加速
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

void updateTopBurst() {
  if (!topBurstActive) return;

  refreshTopHomes();

  unsigned long now = millis();
  unsigned long elapsed = now - topBurstStartTime;

  // 摇晃期间，间隔性轻轻搅动
  if (impulseShaking && !topBurstReturning) {
    if (now - lastTopNudgeTime > TOP_NUDGE_INTERVAL_MS) {
      kickTopParticles(motionLevel + 0.20);
      topBurstFreeUntil = now + TOP_FREE_AFTER_KICK_MS;
      lastTopNudgeTime = now;
    }
  }

  // 最后一次搅动后自由滑动一段时间，再回归
  if (!topBurstReturning) {
    if (now > topBurstFreeUntil || elapsed > TOP_FORCE_RETURN_MS) {
      topBurstReturning = true;
    }
  }

  if (topBurstReturning) {
    bool allHome = true;

    for (int i = 0; i < 3; i++) {
      TopParticle &p = topParticles[i];

      // 慢慢吸回原位
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

  // 自由飞行阶段：无重力、慢速、碰边回弹
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

    // 左右边界
    if (p.x < minX) {
      p.x = minX;
      p.vx = fabsf(p.vx) * TOP_BOUNCE;
    }

    if (p.x > maxX) {
      p.x = maxX;
      p.vx = -fabsf(p.vx) * TOP_BOUNCE;
    }

    // 上下边界
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


// ======================================================
// 摇摆偏移
// ======================================================

int getImpulseShakeOffset() {
  if (!impulseShaking) return 0;

  unsigned long now = millis();
  float elapsed = (float)(now - impulseShakeStartTime);
  float total = (float)IMPULSE_SHAKE_DURATION_MS;

  if (elapsed < 0) elapsed = 0;
  if (elapsed > total) elapsed = total;

  float envelope = 1.0 - elapsed / total;
  float speed = 0.035;

  float x = sinf(elapsed * speed) * impulseAmplitude * envelope;

  return (int)roundf(x);
}

int getWalkingSwayOffset() {
  if (walkSwayAmpSmooth < 0.3) return 0;

  unsigned long now = millis();
  float elapsed = (float)(now - walkingSwayStartTime);

  float speed = 0.0062;
  float x = sinf(elapsed * speed) * walkSwayAmpSmooth;

  return (int)roundf(x);
}

int getTotalShakeOffset() {
  int impulseX = getImpulseShakeOffset();
  int walkX = getWalkingSwayOffset();

  int total = impulseX + walkX;

  if (total > 12) total = 12;
  if (total < -12) total = -12;

  return total;
}


// ======================================================
// 眼睛选择
// ======================================================

int chooseEyeState() {
  unsigned long now = millis();

  if (impulseShaking) {
    return EYE_SQUEEZE;
  }

  if (now < blinkUntil) {
    return EYE_CLOSED;
  }

  if (currentState == STATE_WILTED) {
    return EYE_CLOSED;
  }

  if (walkingSway) {
    return EYE_CENTER;
  }

  if (currentState == STATE_SUNLIGHT) {
    return EYE_UP;
  }

  if (now > nextBlinkTime) {
    blinkUntil = now + 160;
    nextBlinkTime = now + random(5000, 12000);
    return EYE_CLOSED;
  }

  return EYE_CENTER;
}


// ======================================================
// 绘制辅助函数
// ======================================================

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

  sproutCanvas.fillCircle(cx,              cy - spread,      petalR, COLOR_FLOWER_PINK);
  sproutCanvas.fillCircle(cx - spread,     cy - 1,           petalR, COLOR_FLOWER_PINK);
  sproutCanvas.fillCircle(cx + spread,     cy - 1,           petalR, COLOR_FLOWER_PINK);
  sproutCanvas.fillCircle(cx - spread / 2, cy + spread - 1,  petalR, COLOR_FLOWER_PINK);
  sproutCanvas.fillCircle(cx + spread / 2, cy + spread - 1,  petalR, COLOR_FLOWER_PINK);

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


// ======================================================
// 绘制植物
// ======================================================

void drawPlant(int state, int lift, int shakeX) {
  sproutCanvas.fillScreen(COLOR_BG);

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

  uint16_t plantColor = PLANT_GREEN;

  if (currentState == STATE_WILTED) {
    plantColor = COLOR_DIM_GREEN;
  }

  sproutCanvas.fillRoundRect(x1, y1, w1, 16, 4, plantColor);
  sproutCanvas.fillRoundRect(x2, y2, w2, 16, 4, plantColor);
  sproutCanvas.fillRoundRect(x3, y3, w3, 16, 4, plantColor);
  sproutCanvas.fillRoundRect(x4, y4, w4, 12, 4, plantColor);

  int leftFlowerY  = 50 - (lift * 3) / 4;
  int rightFlowerY = 50 - (lift * 3) / 4;
  int midFlowerY   = 45 - lift;

  if (topBurstActive) {
    drawFlyingTopObjects();
  } else {
    if (currentState == STATE_SUNLIGHT) {
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
  }

  int eyeY = 80 - lift / 4;
  int eyeShake = shakeX / 3;

  if (state == EYE_SQUEEZE) {
    drawGreaterEye(50 + eyeShake, eyeY);
    drawLessEye(78 + eyeShake, eyeY);
  }
  else if (state == EYE_CLOSED) {
    sproutCanvas.fillRect(44 + eyeShake, eyeY, 12, 4, DARK_GREEN);
    sproutCanvas.fillRect(72 + eyeShake, eyeY, 12, 4, DARK_GREEN);
  }
  else {
    sproutCanvas.fillCircle(50 + eyeShake, eyeY, 8, FLOWER_WHITE);
    sproutCanvas.fillCircle(78 + eyeShake, eyeY, 8, FLOWER_WHITE);

    int offsetX = 0;
    int offsetY = 0;

    if (state == EYE_RIGHT) offsetX = 3;
    if (state == EYE_UP)    offsetY = -3;
    if (state == EYE_DOWN)  offsetY = 3;

    sproutCanvas.fillCircle(50 + eyeShake + offsetX, eyeY + offsetY, 4, DARK_GREEN);
    sproutCanvas.fillCircle(78 + eyeShake + offsetX, eyeY + offsetY, 4, DARK_GREEN);
  }

#if SHOW_DEBUG_TEXT
  sproutCanvas.setTextSize(1);
  sproutCanvas.setTextColor(COLOR_TEXT, COLOR_BG);

  sproutCanvas.setCursor(2, 2);

  if (dlightReady) {
    sproutCanvas.print(plantStateToText(currentState));
    sproutCanvas.print(" ");
    sproutCanvas.print((int)smoothLux);
    sproutCanvas.print("lx");
  } else {
    sproutCanvas.print("NO DLIGHT");
  }

  sproutCanvas.setCursor(2, 116);
  sproutCanvas.print("h:");
  sproutCanvas.print(lift);

  sproutCanvas.print(" m:");
  sproutCanvas.print(motionLevel, 2);

  if (walkingSway) {
    sproutCanvas.print(" W");
  }

  if (impulseShaking) {
    sproutCanvas.print(" I");
  }

  if (topBurstActive) {
    sproutCanvas.print(" P");
  }
#endif

  sproutCanvas.pushSprite(0, 0);
}


// ======================================================
// 颜色初始化
// ======================================================

void initColors() {
  COLOR_BG = M5.Display.color565(12, 16, 14);
  COLOR_TEXT = M5.Display.color565(230, 240, 220);
  COLOR_SUN = M5.Display.color565(255, 205, 80);
  COLOR_DIM_GREEN = M5.Display.color565(55, 105, 58);

  COLOR_FLOWER_PINK = M5.Display.color565(238, 176, 196);
  COLOR_FLOWER_CENTER = M5.Display.color565(245, 220, 150);
}


// ======================================================
// setup / loop
// ======================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Display.setRotation(0);
  M5.Display.setBrightness(120);
  M5.Display.fillScreen(TFT_BLACK);

  sproutCanvas.setColorDepth(16);
  sproutCanvas.createSprite(M5.Display.width(), M5.Display.height());

  initColors();

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  dlightReady = initBH1750();

  randomSeed(millis());

  nextBlinkTime = millis() + random(5000, 12000);

  Serial.println("City Sprout DLight + Motion + Slow Glass Balls started.");
}

void loop() {
  M5.update();

  unsigned long now = millis();

  if (now - lastSensorTime >= SENSOR_INTERVAL_MS) {
    updateLightState();
    lastSensorTime = now;
  }

  if (now - lastImuTime >= IMU_INTERVAL_MS) {
    updateMotionState();
    lastImuTime = now;
  }

  updateTopBurst();

  if (now - lastDrawTime >= DRAW_INTERVAL_MS) {
    int lift = (int)round(plantLiftSmooth);
    int eyeState = chooseEyeState();
    int shakeX = getTotalShakeOffset();

    drawPlant(eyeState, lift, shakeX);

    frame++;
    lastDrawTime = now;
  }

  if (now - lastPrintTime >= PRINT_INTERVAL_MS) {
    Serial.print("lux raw=");
    Serial.print(lastLux);
    Serial.print(" smooth=");
    Serial.print(smoothLux);
    Serial.print(" state=");
    Serial.print(plantStateToText(currentState));
    Serial.print(" lift=");
    Serial.print(plantLiftSmooth);
    Serial.print(" motion=");
    Serial.print(motionLevel);
    Serial.print(" walking=");
    Serial.print(walkingSway ? "YES" : "NO");
    Serial.print(" impulse=");
    Serial.print(impulseShaking ? "YES" : "NO");
    Serial.print(" particles=");
    Serial.println(topBurstActive ? "YES" : "NO");

    lastPrintTime = now;
  }
}