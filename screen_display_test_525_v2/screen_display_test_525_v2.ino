/*
  出走小芽 Demo - 双屏正确分工版 v2

  屏幕分工：
  1. OLED 黑白屏：
     只显示植物状态和植物说的话

  2. AtomS3R 自带彩屏：
     显示彩色植物本体动画

  保留逻辑：
  - 4 个状态自动轮播
  - 普通呼吸
  - 缺光低头
  - 晒太阳开心
  - 走路摇摆

  改进：
  - OLED 文案变短，不再被切掉
  - OLED 删除 AI Plant
  - M5 彩屏植物继续保持较好看的大叶片版本
*/

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <M5Unified.h>


// ========== 1. OLED I2C 引脚 ==========

// 你之前已经验证过：AtomS3R Grove 口用 G2 / G1 控制 OLED
#define SDA_PIN 2
#define SCL_PIN 1


// ========== 2. OLED 对象 ==========

// 保持你原来正确的 OLED 配置：
// SH1107，64x128，R0 竖屏。
U8G2_SH1107_64X128_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE,
  SCL_PIN,
  SDA_PIN
);


// ========== 3. 动画变量 ==========

// frame 用来控制呼吸、摇摆动画
int frame = 0;


// ========== 4. M5 彩屏颜色变量 ==========

uint16_t COLOR_BG;
uint16_t COLOR_STEM;
uint16_t COLOR_LEAF;
uint16_t COLOR_LEAF_LIGHT;
uint16_t COLOR_LEAF_DARK;
uint16_t COLOR_SUN;
uint16_t COLOR_TEXT;


// ========== 5. OLED 显示文字 ==========

void showOLEDText(int mood) {
  /*
    OLED 只负责显示状态和植物说的话。

    重要：
    OLED 是 64x128 竖屏。
    宽度很窄，所以每行文字必须短。
    不再显示 AI Plant。
  */

  // 每次刷新前清空 OLED 缓冲区
  u8g2.clearBuffer();

  // 使用较小字体，适合 64 像素宽的竖屏
  u8g2.setFont(u8g2_font_5x8_tr);

  // 顶部标题
  u8g2.drawStr(4, 10, "City");
  u8g2.drawStr(4, 20, "Sprout");

  // 第一条分割线
  u8g2.drawHLine(0, 28, 64);

  // 根据不同状态显示不同内容
  if (mood == 0) {
    // 普通呼吸状态
    u8g2.drawStr(4, 44, "Status");
    u8g2.drawStr(4, 56, "Breath");

    u8g2.drawHLine(0, 68, 64);

    u8g2.drawStr(4, 86, "I am");
    u8g2.drawStr(4, 98, "here.");
  }

  else if (mood == 1) {
    // 缺光状态
    u8g2.drawStr(4, 44, "Status");
    u8g2.drawStr(4, 56, "Need sun");

    u8g2.drawHLine(0, 68, 64);

    u8g2.drawStr(4, 86, "Too dark");
    u8g2.drawStr(4, 98, "here...");
  }

  else if (mood == 2) {
    // 晒太阳状态
    u8g2.drawStr(4, 44, "Status");
    u8g2.drawStr(4, 56, "Sun +20");

    u8g2.drawHLine(0, 68, 64);

    u8g2.drawStr(4, 86, "I feel");
    u8g2.drawStr(4, 98, "alive.");
  }

  else if (mood == 3) {
    // 走路状态
    u8g2.drawStr(4, 44, "Status");
    u8g2.drawStr(4, 56, "Walking");

    u8g2.drawHLine(0, 68, 64);

    u8g2.drawStr(4, 86, "Are we");
    u8g2.drawStr(4, 98, "outside?");
  }

  // 底部不再显示 AI Plant
  // 留白，让画面更干净

  // 发送到 OLED
  u8g2.sendBuffer();
}


// ========== 6. M5 彩屏画植物 ==========

void drawPlantOnM5(int mood) {
  /*
    M5 彩屏负责显示真正的植物本体。

    mood:
    0 = 普通呼吸
    1 = 缺光低头
    2 = 晒太阳开心
    3 = 走路摇摆
  */

  // 清空彩屏背景
  M5.Display.fillScreen(COLOR_BG);


  // ========== 6.1 计算动画偏移 ==========

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


  // ========== 6.2 基础坐标 ==========

  int centerX = 64;
  int baseY = 112;
  int topY = 42;
  int leafY = 68;
  int leafDrop = 0;
  int plantSway = sway;


  // ========== 6.3 根据状态调整姿态 ==========

  if (mood == 1) {
    // 缺光：矮一点，叶片下垂
    topY = 58;
    leafY = 76;
    leafDrop = 12;
    plantSway = 0;
  }

  else if (mood == 2) {
    // 晒太阳：长高，叶片展开
    topY = 32 + breathe;
    leafY = 62;
    leafDrop = -6;
    plantSway = sway;
  }

  else if (mood == 3) {
    // 走路：左右摆动更明显
    topY = 42;
    leafY = 68;
    leafDrop = 0;
    plantSway = sway * 3;
  }

  else {
    // 普通呼吸
    topY = 42 + breathe;
    leafY = 68;
    leafDrop = 0;
    plantSway = sway;
  }


  // ========== 6.4 选择叶片颜色 ==========

  uint16_t leafColor = COLOR_LEAF;

  if (mood == 1) {
    leafColor = COLOR_LEAF_DARK;
  } else if (mood == 2) {
    leafColor = COLOR_LEAF_LIGHT;
  }


  // ========== 6.5 画阳光 / 动线 ==========

  if (mood == 2) {
    // 晒太阳状态：右上角画太阳和光点
    M5.Display.fillCircle(104, 22, 7, COLOR_SUN);
    M5.Display.fillCircle(92, 36, 2, COLOR_SUN);
    M5.Display.fillCircle(113, 42, 2, COLOR_SUN);
    M5.Display.fillCircle(88, 20, 1, COLOR_SUN);
  }

  if (mood == 3) {
    // 走路状态：两侧画动线
    M5.Display.drawLine(8, 52, 22, 52, COLOR_TEXT);
    M5.Display.drawLine(106, 68, 124, 68, COLOR_TEXT);
  }


  // ========== 6.6 画茎 ==========

  // 主茎：用多条线做得更粗一点
  M5.Display.drawLine(centerX - 2, baseY, centerX + plantSway, topY, COLOR_STEM);
  M5.Display.drawLine(centerX - 1, baseY, centerX + plantSway + 1, topY, COLOR_STEM);
  M5.Display.drawLine(centerX, baseY, centerX + plantSway + 2, topY, COLOR_STEM);


  // ========== 6.7 画左叶片 ==========

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


  // ========== 6.8 画右叶片 ==========

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


  // ========== 6.9 顶部新芽 ==========

  int budX = centerX + plantSway + 8;
  int budY = topY + 4;

  if (mood == 1) {
    // 缺光：顶部小芽收缩
    M5.Display.fillCircle(budX, budY + 4, 5, leafColor);
  }

  else if (mood == 2) {
    // 晒太阳：顶部小芽展开
    M5.Display.fillEllipse(budX, budY, 11, 6, leafColor);
    M5.Display.drawLine(centerX + plantSway, topY + 8, budX, budY, COLOR_STEM);
  }

  else {
    // 普通 / 走路
    M5.Display.fillEllipse(budX, budY, 9, 5, leafColor);
    M5.Display.drawLine(centerX + plantSway, topY + 8, budX, budY, COLOR_STEM);
  }


  // ========== 6.10 画花盆 / 土壤 ==========

  M5.Display.fillRoundRect(
    42,
    112,
    44,
    10,
    5,
    M5.Display.color565(90, 60, 40)
  );

  M5.Display.drawRoundRect(
    40,
    110,
    48,
    14,
    5,
    COLOR_TEXT
  );


  // ========== 6.11 小状态标签 ==========

  M5.Display.setTextSize(1);
  M5.Display.setTextColor(COLOR_TEXT, COLOR_BG);
  M5.Display.setCursor(4, 4);

  if (mood == 0) {
    M5.Display.print("breathing");
  } else if (mood == 1) {
    M5.Display.print("need sun");
  } else if (mood == 2) {
    M5.Display.print("sun +20");
  } else if (mood == 3) {
    M5.Display.print("walking");
  }
}


// ========== 7. setup ==========

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 初始化 M5
  auto cfg = M5.config();
  M5.begin(cfg);

  // 初始化 M5 彩屏颜色
  COLOR_BG = M5.Display.color565(12, 16, 14);
  COLOR_STEM = M5.Display.color565(210, 230, 200);
  COLOR_LEAF = M5.Display.color565(70, 160, 80);
  COLOR_LEAF_LIGHT = M5.Display.color565(110, 220, 95);
  COLOR_LEAF_DARK = M5.Display.color565(48, 85, 55);
  COLOR_SUN = M5.Display.color565(255, 205, 80);
  COLOR_TEXT = M5.Display.color565(230, 240, 220);

  // 启动 I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // 启动 OLED
  u8g2.begin();
  u8g2.setContrast(255);
  u8g2.clearDisplay();

  Serial.println("Dual screen sprout demo v2 start.");
}


// ========== 8. loop ==========

void loop() {
  /*
    状态轮播：

    前 5 秒：普通呼吸
    5-10 秒：缺光低头
    10-15 秒：晒太阳开心
    15-20 秒：走路摇摆
  */

  M5.update();

  unsigned long now = millis();

  int mood = (now / 5000) % 4;

  // OLED：只显示文字
  showOLEDText(mood);

  // M5：显示彩色植物
  drawPlantOnM5(mood);

  frame++;

  delay(100);
}