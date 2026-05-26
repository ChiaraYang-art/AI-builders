/*
  出走小芽 Demo - DLight + IMU + M5 彩色植物合并版

  当前硬件：
  1. AtomS3R
  2. DLight / BH1750 光照传感器

  当前连接：
  AtomS3R Grove口 → DLight 光照传感器

  功能：
  1. DLight 读取真实光照 lux
  2. IMU 判断设备是否被移动
  3. M5 自带彩屏显示彩色植物
  4. 光照控制植物情绪
  5. 运动控制植物动作
  6. Serial Monitor 打印调试信息

  不需要 OLED。
  等分线器到了，再把 OLED 文案显示合并进来。
*/

#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <math.h>


// =====================================================
// 1. DLight / BH1750 设置
// =====================================================

// AtomS3R Grove 口 I2C 引脚
#define SDA_PIN 2
#define SCL_PIN 1

// 你之前已经扫描到 DLight 地址是 0x23
#define BH1750_ADDR 0x23


// =====================================================
// 2. 光照阈值设置
// =====================================================

/*
  根据你们实测数据：

  手捂住：0 lux
  桌面平放：约 447 lux
  手电筒 20cm：约 530 lux
  手电筒 10cm：约 915 lux

  当前阈值：

  0 - 50 lux       NEED SUN
  50 - 500 lux     INDOOR
  500 - 800 lux    BRIGHT
  800+ lux         SUNLIGHT
*/

#define LUX_NEED_SUN_MAX 50
#define LUX_INDOOR_MAX   500
#define LUX_BRIGHT_MAX   800


// =====================================================
// 3. IMU 运动阈值设置
// =====================================================

/*
  两态：

  STILL:
  静止

  ACTIVE:
  被拿起来、移动、走路、晃动

  ACTIVE_THRESHOLD 越小越敏感。
  如果放桌上也容易 ACTIVE，就改大一点，比如 0.08。
  如果拿起来不够灵敏，就改小一点，比如 0.04。
*/

const float ACTIVE_THRESHOLD = 0.06;

// 检测到 ACTIVE 后保持 1.5 秒
const unsigned long ACTIVE_HOLD_TIME = 1500;

unsigned long lastActiveTime = 0;


// =====================================================
// 4. Serial Monitor 输出控制
// =====================================================

unsigned long lastPrintTime = 0;

// 每 800ms 打印一次
const unsigned long PRINT_INTERVAL = 800;


// =====================================================
// 5. 状态定义
// =====================================================

enum LightMood {
  LIGHT_NEED_SUN = 0,
  LIGHT_INDOOR   = 1,
  LIGHT_BRIGHT   = 2,
  LIGHT_SUNLIGHT = 3
};

enum MotionState {
  MOTION_STILL  = 0,
  MOTION_ACTIVE = 1
};

LightMood currentLightMood = LIGHT_INDOOR;
MotionState currentMotion = MOTION_STILL;


// =====================================================
// 6. 动画变量
// =====================================================

int frame = 0;


// =====================================================
// 7. 颜色变量
// =====================================================

uint16_t COLOR_BG;
uint16_t COLOR_STEM;
uint16_t COLOR_LEAF;
uint16_t COLOR_LEAF_LIGHT;
uint16_t COLOR_LEAF_DARK;
uint16_t COLOR_SUN;
uint16_t COLOR_TEXT;
uint16_t COLOR_POT;


// =====================================================
// 8. BH1750 基础函数
// =====================================================

void bh1750WriteCommand(byte command) {
  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(command);
  Wire.endTransmission();
}


float readLightLux() {
  /*
    BH1750 返回 2 个字节。
    lux = raw / 1.2
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


// =====================================================
// 9. 根据 lux 判断光照状态
// =====================================================

LightMood getLightMoodFromLux(float lux) {
  if (lux < LUX_NEED_SUN_MAX) {
    return LIGHT_NEED_SUN;
  }

  if (lux < LUX_INDOOR_MAX) {
    return LIGHT_INDOOR;
  }

  if (lux < LUX_BRIGHT_MAX) {
    return LIGHT_BRIGHT;
  }

  return LIGHT_SUNLIGHT;
}


const char* lightMoodToText(LightMood mood) {
  if (mood == LIGHT_NEED_SUN) {
    return "NEED_SUN";
  }

  if (mood == LIGHT_INDOOR) {
    return "INDOOR";
  }

  if (mood == LIGHT_BRIGHT) {
    return "BRIGHT";
  }

  if (mood == LIGHT_SUNLIGHT) {
    return "SUNLIGHT";
  }

  return "UNKNOWN";
}


// =====================================================
// 10. IMU 运动判断
// =====================================================

float getMotionStrength(float ax, float ay, float az) {
  /*
    静止时：
    sqrt(ax^2 + ay^2 + az^2) 接近 1

    运动时：
    总加速度会偏离 1

    motionStrength 越大，说明运动越明显。
  */

  float totalAcc = sqrt(ax * ax + ay * ay + az * az);

  float motionStrength = abs(totalAcc - 1.0);

  return motionStrength;
}


MotionState updateMotionState(float motionStrength) {
  /*
    两态稳定判断：

    1. 当前运动强度超过阈值 → ACTIVE
    2. 最近 1.5 秒内检测到 ACTIVE → 继续保持 ACTIVE
    3. 否则 → STILL
  */

  unsigned long now = millis();

  if (motionStrength > ACTIVE_THRESHOLD) {
    lastActiveTime = now;
    return MOTION_ACTIVE;
  }

  if (now - lastActiveTime < ACTIVE_HOLD_TIME) {
    return MOTION_ACTIVE;
  }

  return MOTION_STILL;
}


const char* motionToText(MotionState motion) {
  if (motion == MOTION_STILL) {
    return "STILL";
  }

  if (motion == MOTION_ACTIVE) {
    return "ACTIVE";
  }

  return "UNKNOWN";
}


// =====================================================
// 11. 画植物
// =====================================================

void drawPlantOnM5(LightMood lightMood, MotionState motion, float lux) {
  /*
    M5 屏幕负责显示植物本体。

    光照影响：
    NEED SUN  → 低头、暗绿色
    INDOOR    → 普通呼吸
    BRIGHT    → 更精神
    SUNLIGHT  → 开心、有太阳

    运动影响：
    STILL  → 轻微呼吸
    ACTIVE → 左右摆动更明显
  */

  M5.Display.fillScreen(COLOR_BG);


  // -----------------------------
  // 11.1 基础动画参数
  // -----------------------------

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


  // -----------------------------
  // 11.2 基础坐标
  // -----------------------------

  int centerX = 64;
  int baseY = 112;

  int topY = 44;
  int leafY = 70;
  int leafDrop = 0;
  int plantSway = sway;

  uint16_t leafColor = COLOR_LEAF;


  // -----------------------------
  // 11.3 光照控制植物情绪
  // -----------------------------

  if (lightMood == LIGHT_NEED_SUN) {
    // 缺光：矮一点，低头，叶片下垂
    topY = 62;
    leafY = 80;
    leafDrop = 12;
    leafColor = COLOR_LEAF_DARK;
  }

  else if (lightMood == LIGHT_INDOOR) {
    // 普通室内光：正常呼吸
    topY = 44 + breathe;
    leafY = 70;
    leafDrop = 0;
    leafColor = COLOR_LEAF;
  }

  else if (lightMood == LIGHT_BRIGHT) {
    // 明亮：更精神一点
    topY = 38 + breathe;
    leafY = 66;
    leafDrop = -2;
    leafColor = COLOR_LEAF_LIGHT;
  }

  else if (lightMood == LIGHT_SUNLIGHT) {
    // 强光：最开心，长高，叶片展开
    topY = 32 + breathe;
    leafY = 62;
    leafDrop = -6;
    leafColor = COLOR_LEAF_LIGHT;
  }


  // -----------------------------
  // 11.4 IMU 控制动作幅度
  // -----------------------------

  if (motion == MOTION_ACTIVE) {
    /*
      ACTIVE 时：
      小芽左右摆动更明显。
      表示它被用户拿起来/带着走。
    */
    plantSway = sway * 3;
  } else {
    /*
      STILL 时：
      只做轻微呼吸。
    */
    plantSway = sway;
  }

  if (lightMood == LIGHT_NEED_SUN && motion == MOTION_STILL) {
    /*
      缺光且静止时，不要左右摇太明显，
      更像低落地待着。
    */
    plantSway = 0;
  }


  // -----------------------------
  // 11.5 氛围元素：太阳 / 动线
  // -----------------------------

  if (lightMood == LIGHT_SUNLIGHT) {
    // 右上角太阳
    M5.Display.fillCircle(104, 22, 7, COLOR_SUN);

    // 小光点
    M5.Display.fillCircle(92, 36, 2, COLOR_SUN);
    M5.Display.fillCircle(113, 42, 2, COLOR_SUN);
    M5.Display.fillCircle(88, 20, 1, COLOR_SUN);
  }

  if (motion == MOTION_ACTIVE) {
    // ACTIVE 时加一点动线
    M5.Display.drawLine(8, 54, 22, 54, COLOR_TEXT);
    M5.Display.drawLine(104, 72, 122, 72, COLOR_TEXT);
  }

  if (lightMood == LIGHT_NEED_SUN) {
    // 缺光时三个小点
    M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(50, 18);
    M5.Display.print("...");
  }


  // -----------------------------
  // 11.6 画茎
  // -----------------------------

  M5.Display.drawLine(centerX - 2, baseY, centerX + plantSway,     topY, COLOR_STEM);
  M5.Display.drawLine(centerX - 1, baseY, centerX + plantSway + 1, topY, COLOR_STEM);
  M5.Display.drawLine(centerX,     baseY, centerX + plantSway + 2, topY, COLOR_STEM);


  // -----------------------------
  // 11.7 左叶片
  // -----------------------------

  int leftLeafX = centerX - 26 + plantSway;
  int leftLeafY = leafY + leafDrop;

  M5.Display.fillEllipse(
    leftLeafX,
    leftLeafY,
    24,
    11,
    leafColor
  );

  // 左叶叶脉
  M5.Display.drawLine(
    centerX + plantSway,
    leafY,
    leftLeafX - 18,
    leftLeafY,
    COLOR_STEM
  );

  M5.Display.drawLine(
    leftLeafX - 4,
    leftLeafY,
    leftLeafX - 12,
    leftLeafY - 5,
    COLOR_STEM
  );

  M5.Display.drawLine(
    leftLeafX - 4,
    leftLeafY,
    leftLeafX - 12,
    leftLeafY + 5,
    COLOR_STEM
  );


  // -----------------------------
  // 11.8 右叶片
  // -----------------------------

  int rightLeafX = centerX + 26 + plantSway;
  int rightLeafY = leafY + leafDrop + 4;

  M5.Display.fillEllipse(
    rightLeafX,
    rightLeafY,
    24,
    11,
    leafColor
  );

  // 右叶叶脉
  M5.Display.drawLine(
    centerX + plantSway,
    leafY + 4,
    rightLeafX + 18,
    rightLeafY,
    COLOR_STEM
  );

  M5.Display.drawLine(
    rightLeafX + 4,
    rightLeafY,
    rightLeafX + 12,
    rightLeafY - 5,
    COLOR_STEM
  );

  M5.Display.drawLine(
    rightLeafX + 4,
    rightLeafY,
    rightLeafX + 12,
    rightLeafY + 5,
    COLOR_STEM
  );


  // -----------------------------
  // 11.9 顶部新芽
  // -----------------------------

  int budX = centerX + plantSway + 8;
  int budY = topY + 4;

  if (lightMood == LIGHT_NEED_SUN) {
    // 缺光：顶部小芽收缩
    M5.Display.fillCircle(budX, budY + 4, 5, leafColor);
  }

  else if (lightMood == LIGHT_SUNLIGHT) {
    // 强光：顶部小芽展开
    M5.Display.fillEllipse(budX, budY, 12, 6, leafColor);

    M5.Display.drawLine(
      centerX + plantSway,
      topY + 8,
      budX,
      budY,
      COLOR_STEM
    );
  }

  else {
    // 普通 / 明亮
    M5.Display.fillEllipse(budX, budY, 9, 5, leafColor);

    M5.Display.drawLine(
      centerX + plantSway,
      topY + 8,
      budX,
      budY,
      COLOR_STEM
    );
  }


  // -----------------------------
  // 11.10 花盆
  // -----------------------------

  M5.Display.fillRoundRect(
    42,
    112,
    44,
    10,
    5,
    COLOR_POT
  );

  M5.Display.drawRoundRect(
    40,
    110,
    48,
    14,
    5,
    COLOR_TEXT
  );


  // -----------------------------
  // 11.11 很小的调试信息
  // -----------------------------
  /*
    这里不是最终 UI，只是为了调试方便。
    如果你完全不想在 M5 上看到文字，
    可以把下面这一整段注释掉。
  */

  M5.Display.setTextSize(1);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);

  M5.Display.setCursor(4, 4);
  M5.Display.print((int)lux);
  M5.Display.print("lx");

  M5.Display.setCursor(4, 16);

  if (motion == MOTION_ACTIVE) {
    M5.Display.print("active");
  } else {
    M5.Display.print("still");
  }
}


// =====================================================
// 12. setup
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 初始化 M5
  auto cfg = M5.config();
  M5.begin(cfg);

  // 初始化颜色
  COLOR_BG = M5.Display.color565(12, 16, 14);
  COLOR_STEM = M5.Display.color565(210, 230, 200);
  COLOR_LEAF = M5.Display.color565(70, 160, 80);
  COLOR_LEAF_LIGHT = M5.Display.color565(110, 220, 95);
  COLOR_LEAF_DARK = M5.Display.color565(48, 85, 55);
  COLOR_SUN = M5.Display.color565(255, 205, 80);
  COLOR_TEXT = M5.Display.color565(230, 240, 220);
  COLOR_POT = M5.Display.color565(90, 60, 40);

  // 初始化 I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // 初始化 BH1750
  bh1750WriteCommand(0x01);  // Power On
  delay(10);

  bh1750WriteCommand(0x10);  // Continuous High Resolution Mode
  delay(200);

  // 开机提示
  M5.Display.fillScreen(COLOR_BG);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(8, 8);
  M5.Display.print("Sprout demo");
  M5.Display.setCursor(8, 24);
  M5.Display.print("DLight + IMU");

  Serial.println();
  Serial.println("DLight + IMU + M5 plant demo start");
  Serial.println("--------------------------------");
  Serial.println("Light thresholds:");
  Serial.println("0-50    NEED_SUN");
  Serial.println("50-500  INDOOR");
  Serial.println("500-800 BRIGHT");
  Serial.println("800+    SUNLIGHT");
  Serial.println("--------------------------------");
  Serial.println("Motion:");
  Serial.println("STILL / ACTIVE");
  Serial.println("--------------------------------");
}


// =====================================================
// 13. loop
// =====================================================

void loop() {
  M5.update();

  // -----------------------------
  // 13.1 读取 DLight
  // -----------------------------

  float lux = readLightLux();

  if (lux < 0) {
    // 读取失败时显示错误
    M5.Display.fillScreen(COLOR_BG);
    M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Display.setTextSize(1);
    M5.Display.setCursor(8, 8);
    M5.Display.print("DLight read fail");

    Serial.println("Failed to read DLight.");
    delay(500);
    return;
  }

  currentLightMood = getLightMoodFromLux(lux);


  // -----------------------------
  // 13.2 读取 IMU
  // -----------------------------

  float ax = 0;
  float ay = 0;
  float az = 0;

  M5.Imu.getAccelData(&ax, &ay, &az);

  float motionStrength = getMotionStrength(ax, ay, az);

  currentMotion = updateMotionState(motionStrength);


  // -----------------------------
  // 13.3 更新 M5 植物显示
  // -----------------------------

  drawPlantOnM5(currentLightMood, currentMotion, lux);


  // -----------------------------
  // 13.4 Serial Monitor 调试输出
  // -----------------------------

  if (millis() - lastPrintTime > PRINT_INTERVAL) {

    Serial.print("Lux: ");
    Serial.print(lux);
    Serial.print(" lx");

    Serial.print("  |  Light: ");
    Serial.print(lightMoodToText(currentLightMood));

    Serial.print("  |  Motion: ");
    Serial.print(motionToText(currentMotion));

    Serial.print("  |  strength: ");
    Serial.print(motionStrength, 3);

    Serial.print("  |  ax:");
    Serial.print(ax, 2);

    Serial.print(" ay:");
    Serial.print(ay, 2);

    Serial.print(" az:");
    Serial.println(az, 2);

    Serial.println("--------------------------------");

    lastPrintTime = millis();
  }

  frame++;

  delay(120);
}