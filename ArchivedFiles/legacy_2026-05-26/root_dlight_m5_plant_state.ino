/*
  出走小芽 Demo - DLight 控制 M5 彩色植物状态版

  当前硬件：
  AtomS3R
  ↓ Grove线
  DLight 光照传感器 BH1750

  功能：
  1. 读取 DLight 光照 lux 数值
  2. 根据 lux 判断植物状态
  3. 在 AtomS3R 自带彩屏上显示彩色植物
  4. Serial Monitor 同时打印 lux 和状态

  当前不接 OLED。
  等分线器到了，再把 OLED 文案显示合并回来。
*/

#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>


// ========== 1. DLight / BH1750 设置 ==========

// AtomS3R Grove 口 I2C 引脚
#define SDA_PIN 2
#define SCL_PIN 1

// 你已经用 I2C Scanner 扫到 DLight 地址是 0x23
#define BH1750_ADDR 0x23


// ========== 2. 光照阈值设置 ==========

/*
  根据你刚刚实测的数据：

  手捂住：0 lux
  桌面平放：约 447 lux
  手电筒 20cm：约 530 lux
  手电筒 10cm：约 915 lux

  所以先用这一版阈值：

  0-50 lux      NEED SUN
  50-500 lux    INDOOR
  500-800 lux   BRIGHT
  800+ lux      SUNLIGHT
*/

#define LUX_NEED_SUN_MAX 50
#define LUX_INDOOR_MAX   500
#define LUX_BRIGHT_MAX   800


// ========== 3. 植物状态 ==========

enum PlantMood {
  MOOD_NEED_SUN = 0,
  MOOD_INDOOR   = 1,
  MOOD_BRIGHT   = 2,
  MOOD_SUNLIGHT = 3
};

PlantMood currentMood = MOOD_INDOOR;


// ========== 4. 动画变量 ==========

int frame = 0;


// ========== 5. 颜色变量 ==========

uint16_t COLOR_BG;
uint16_t COLOR_STEM;
uint16_t COLOR_LEAF;
uint16_t COLOR_LEAF_LIGHT;
uint16_t COLOR_LEAF_DARK;
uint16_t COLOR_SUN;
uint16_t COLOR_TEXT;
uint16_t COLOR_POT;


// ========== 6. BH1750 基础函数 ==========

void bh1750WriteCommand(byte command) {
  /*
    给 BH1750 发送命令。

    0x01 = Power On
    0x10 = 连续高分辨率测量模式
  */

  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(command);
  Wire.endTransmission();
}


float readLightLux() {
  /*
    从 BH1750 读取 2 个字节数据。
    BH1750 的换算关系一般是：

    lux = raw / 1.2

    如果读取失败，返回 -1。
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


// ========== 7. 根据 lux 判断植物状态 ==========

PlantMood getMoodFromLux(float lux) {
  /*
    这个函数把 lux 转成植物状态。

    lux < 50:
    太暗，小芽需要太阳

    50-500:
    普通室内光

    500-800:
    明亮，小芽精神起来

    800+:
    强光，小芽晒到太阳
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


// ========== 8. 状态转文字 ==========

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


// ========== 9. 画 M5 彩色植物 ==========

void drawPlantOnM5(PlantMood mood, float lux) {
  /*
    这个函数负责在 AtomS3R 自带彩屏上画植物。

    不同状态对应不同形态：

    NEED SUN:
    暗绿色、低头、叶子下垂

    INDOOR:
    普通呼吸

    BRIGHT:
    叶子更亮、更精神

    SUNLIGHT:
    亮绿色、长高、有太阳光点
  */

  // 先清屏
  M5.Display.fillScreen(COLOR_BG);


  // ========== 9.1 动画参数 ==========

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


  // ========== 9.2 基础坐标 ==========

  int centerX = 64;
  int baseY = 112;

  int topY = 42;
  int leafY = 68;
  int leafDrop = 0;
  int plantSway = sway;

  uint16_t leafColor = COLOR_LEAF;


  // ========== 9.3 根据状态调整植物姿态 ==========

  if (mood == MOOD_NEED_SUN) {
    // 缺光：矮一点，叶子下垂，颜色暗
    topY = 60;
    leafY = 78;
    leafDrop = 12;
    plantSway = 0;
    leafColor = COLOR_LEAF_DARK;
  }

  else if (mood == MOOD_INDOOR) {
    // 室内：普通呼吸
    topY = 44 + breathe;
    leafY = 70;
    leafDrop = 0;
    plantSway = sway;
    leafColor = COLOR_LEAF;
  }

  else if (mood == MOOD_BRIGHT) {
    // 明亮：稍微长高，叶子更亮
    topY = 38 + breathe;
    leafY = 66;
    leafDrop = -2;
    plantSway = sway;
    leafColor = COLOR_LEAF_LIGHT;
  }

  else if (mood == MOOD_SUNLIGHT) {
    // 强光：开心，长高，叶子展开
    topY = 32 + breathe;
    leafY = 62;
    leafDrop = -6;
    plantSway = sway * 2;
    leafColor = COLOR_LEAF_LIGHT;
  }


  // ========== 9.4 氛围元素 ==========

  if (mood == MOOD_SUNLIGHT) {
    // 右上角太阳
    M5.Display.fillCircle(104, 22, 7, COLOR_SUN);

    // 小光点
    M5.Display.fillCircle(92, 36, 2, COLOR_SUN);
    M5.Display.fillCircle(113, 42, 2, COLOR_SUN);
    M5.Display.fillCircle(88, 20, 1, COLOR_SUN);
  }

  if (mood == MOOD_NEED_SUN) {
    // 缺光时画三个小点，表示低落
    M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(50, 18);
    M5.Display.print("...");
  }


  // ========== 9.5 画茎 ==========

  M5.Display.drawLine(centerX - 2, baseY, centerX + plantSway, topY, COLOR_STEM);
  M5.Display.drawLine(centerX - 1, baseY, centerX + plantSway + 1, topY, COLOR_STEM);
  M5.Display.drawLine(centerX,     baseY, centerX + plantSway + 2, topY, COLOR_STEM);


  // ========== 9.6 画左叶子 ==========

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


  // ========== 9.7 画右叶子 ==========

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


  // ========== 9.8 顶部新芽 ==========

  int budX = centerX + plantSway + 8;
  int budY = topY + 4;

  if (mood == MOOD_NEED_SUN) {
    // 缺光：小芽收缩
    M5.Display.fillCircle(budX, budY + 4, 5, leafColor);
  }

  else if (mood == MOOD_SUNLIGHT) {
    // 阳光：顶部小芽展开
    M5.Display.fillEllipse(budX, budY, 12, 6, leafColor);
    M5.Display.drawLine(centerX + plantSway, topY + 8, budX, budY, COLOR_STEM);
  }

  else {
    // 普通 / 明亮
    M5.Display.fillEllipse(budX, budY, 9, 5, leafColor);
    M5.Display.drawLine(centerX + plantSway, topY + 8, budX, budY, COLOR_STEM);
  }


  // ========== 9.9 花盆 ==========

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


  // ========== 9.10 顶部显示 lux 和状态 ==========

  M5.Display.setTextSize(1);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);

  M5.Display.setCursor(4, 4);
  M5.Display.print(moodToText(mood));

  M5.Display.setCursor(4, 16);
  M5.Display.print("Lux:");
  M5.Display.print((int)lux);
}


// ========== 10. setup ==========

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 初始化 M5
  auto cfg = M5.config();
  M5.begin(cfg);

  // 初始化颜色
  COLOR_BG = M5.Display.color565(12, 16, 14);          // 深色背景
  COLOR_STEM = M5.Display.color565(210, 230, 200);     // 浅色茎
  COLOR_LEAF = M5.Display.color565(70, 160, 80);       // 普通绿
  COLOR_LEAF_LIGHT = M5.Display.color565(110, 220, 95);// 亮绿
  COLOR_LEAF_DARK = M5.Display.color565(48, 85, 55);   // 暗绿
  COLOR_SUN = M5.Display.color565(255, 205, 80);       // 阳光黄
  COLOR_TEXT = M5.Display.color565(230, 240, 220);     // 浅色文字
  COLOR_POT = M5.Display.color565(90, 60, 40);         // 花盆棕色

  // 初始化 I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // 初始化 BH1750
  bh1750WriteCommand(0x01);  // Power On
  delay(10);

  bh1750WriteCommand(0x10);  // Continuous High Resolution Mode
  delay(200);

  // 初始屏幕
  M5.Display.fillScreen(COLOR_BG);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(8, 8);
  M5.Display.print("DLight Sprout");
  M5.Display.setCursor(8, 24);
  M5.Display.print("Starting...");

  Serial.println();
  Serial.println("DLight controls M5 plant demo start");
  Serial.println("Thresholds:");
  Serial.println("0-50    NEED SUN");
  Serial.println("50-500  INDOOR");
  Serial.println("500-800 BRIGHT");
  Serial.println("800+    SUNLIGHT");
  Serial.println("--------------------------------");
}


// ========== 11. loop ==========

void loop() {
  M5.update();

  float lux = readLightLux();

  if (lux >= 0) {
    currentMood = getMoodFromLux(lux);

    drawPlantOnM5(currentMood, lux);

    Serial.print("Lux: ");
    Serial.print(lux);
    Serial.print(" lx");
    Serial.print("  |  Plant state: ");
    Serial.println(moodToText(currentMood));
    Serial.println("--------------------------------");
  }

  else {
    M5.Display.fillScreen(COLOR_BG);
    M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
    M5.Display.setTextSize(1);
    M5.Display.setCursor(8, 8);
    M5.Display.print("DLight read fail");

    Serial.println("Failed to read DLight.");
    Serial.println("--------------------------------");
  }

  frame++;

  delay(300);
}