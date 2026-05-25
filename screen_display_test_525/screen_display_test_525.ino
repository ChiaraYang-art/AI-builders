/*
  出走小芽 Demo - OLED 小芽动画测试版

  这个程序做什么？
  1. 控制外接 OLED 屏幕
  2. 在屏幕上画一个“小芽”
  3. 让小芽轻轻摆动，看起来像在呼吸
  4. 底部显示一行状态文字

  当前硬件：
  AtomS3R
  ↓
  外接 OLED 屏幕

  注意：
  这一版不需要光照传感器。
  这一版不需要 Flask。
  这一版只是专门调试 OLED 动画。
*/


#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>


// ========== 1. I2C 引脚设置 ==========

// 你之前已经测试成功：AtomS3R 的 Grove 口用 G2 / G1 控制 OLED
#define SDA_PIN 2
#define SCL_PIN 1


// ========== 2. OLED 屏幕对象 ==========

// 你的 OLED 是 SH1107，原生逻辑是 64x128。
// U8G2_R0 表示竖屏使用。
// 如果你发现画面上下颠倒，可以改成 U8G2_R2。
U8G2_SH1107_64X128_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE,
  SCL_PIN,
  SDA_PIN
);


// ========== 3. 动画变量 ==========

// frame 用来记录当前是第几帧。
// 它会不断增加，我们用它制造动画变化。
int frame = 0;


// ========== 4. 画小芽的函数 ==========

void drawSprout(int mood) {
  /*
    这个函数负责画小芽。

    参数 mood 表示小芽状态：
    0 = 普通呼吸
    1 = 缺光低头
    2 = 晒太阳开心
    3 = 走路摇摆

    你可以把它理解为：
    不同状态下，同一棵小芽的姿势不同。
  */


  // 先清空缓冲区。
  // 每一帧动画都要先清空，再重新画。
  // 不然旧画面会残留，变成乱点点。
  u8g2.clearBuffer();


  // ========== 4.1 计算动画偏移 ==========

  // sway 是左右摇摆的幅度。
  // frame % 20 会得到 0 到 19 的循环数字。
  // 根据不同范围，让它在 -2 到 +2 之间变化。
  int sway = 0;

  int phase = frame % 20;

  if (phase < 5) {
    sway = -2;
  } else if (phase < 10) {
    sway = -1;
  } else if (phase < 15) {
    sway = 1;
  } else {
    sway = 2;
  }


  // breathe 是上下呼吸感。
  // 它会让小芽高度轻微变化。
  int breathe = (frame % 30 < 15) ? 0 : 2;


  // ========== 4.2 根据 mood 调整姿态 ==========

  // 小芽的中心 x 坐标。
  // 竖屏宽度是 64，所以中心大概是 32。
  int centerX = 32;

  // 小芽根部 y 坐标。
  // 竖屏高度是 128，底部大概在 100 附近。
  int baseY = 96;

  // 小芽顶部 y 坐标。
  // 数值越小，位置越靠上。
  int topY = 48;

  // 叶子下垂程度。
  // 数值越大，叶子越往下。
  int leafDrop = 0;

  // 根据状态改变姿态。
  if (mood == 1) {
    // 缺光：小芽矮一点，叶子下垂
    topY = 62;
    leafDrop = 8;
    sway = 0;
  } else if (mood == 2) {
    // 晒太阳：小芽高一点，叶子展开
    topY = 38 + breathe;
    leafDrop = -4;
  } else if (mood == 3) {
    // 走路：左右摆动更明显
    topY = 48;
    leafDrop = 0;
    sway = sway * 2;
  } else {
    // 普通状态：轻微呼吸
    topY = 48 + breathe;
    leafDrop = 0;
  }


  // ========== 4.3 画土壤/花盆底部 ==========

  // drawRBox 是画圆角实心矩形。
  // 参数是：x, y, 宽度, 高度, 圆角半径
  u8g2.drawRBox(18, 98, 28, 8, 3);

  // 画一个小盆的轮廓
  u8g2.drawFrame(16, 104, 32, 10);


  // ========== 4.4 画茎 ==========

  // drawLine 是画线。
  // 从根部到顶部，加入 sway 让小芽看起来在动。
  u8g2.drawLine(centerX, baseY, centerX + sway, topY);


  // 为了让茎更粗一点，再画一条相邻的线
  u8g2.drawLine(centerX + 1, baseY, centerX + sway + 1, topY);


  // ========== 4.5 画左叶子 ==========

  // 左叶子的起点在茎的中上部
  int leafY = topY + 14;

  // 左叶子尖端
  int leftLeafX = centerX - 18 + sway;
  int leftLeafY = leafY + leafDrop;

  // 画左叶子轮廓。
  // 用三条线画成一个简单叶片。
  u8g2.drawLine(centerX + sway, leafY, leftLeafX, leftLeafY);
  u8g2.drawLine(leftLeafX, leftLeafY, leftLeafX + 8, leftLeafY - 5);
  u8g2.drawLine(leftLeafX + 8, leftLeafY - 5, centerX + sway, leafY);


  // ========== 4.6 画右叶子 ==========

  int rightLeafX = centerX + 18 + sway;
  int rightLeafY = leafY + leafDrop;

  u8g2.drawLine(centerX + sway, leafY + 4, rightLeafX, rightLeafY);
  u8g2.drawLine(rightLeafX, rightLeafY, rightLeafX - 8, rightLeafY - 5);
  u8g2.drawLine(rightLeafX - 8, rightLeafY - 5, centerX + sway, leafY + 4);


  // ========== 4.7 根据状态画表情/光点 ==========

  if (mood == 1) {
    // 缺光状态：在小芽旁边画一个小小的低落符号
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(25, 32, "...");
  }

  if (mood == 2) {
    // 晒太阳状态：画几个小光点
    u8g2.drawCircle(50, 22, 3);
    u8g2.drawPixel(46, 18);
    u8g2.drawPixel(55, 18);
    u8g2.drawPixel(46, 26);
    u8g2.drawPixel(55, 26);
  }

  if (mood == 3) {
    // 走路状态：画几条动线
    u8g2.drawLine(8, 45, 14, 45);
    u8g2.drawLine(50, 55, 58, 55);
  }


  // ========== 4.8 底部文字 ==========

  u8g2.setFont(u8g2_font_6x12_tr);

  if (mood == 1) {
    u8g2.drawStr(6, 123, "Need sun");
  } else if (mood == 2) {
    u8g2.drawStr(6, 123, "Sun +20");
  } else if (mood == 3) {
    u8g2.drawStr(6, 123, "Walking");
  } else {
    u8g2.drawStr(6, 123, "City Sprout");
  }


  // ========== 4.9 发送到屏幕 ==========

  // 这句一定要有。
  // 它会把刚才画好的这一帧真正显示到 OLED 上。
  u8g2.sendBuffer();
}


// ========== 5. setup：开机运行一次 ==========

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 启动 I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // 启动 OLED
  u8g2.begin();

  // 设置 OLED 对比度，255 是最亮
  u8g2.setContrast(255);

  // 先清一次屏，避免开机随机点点
  u8g2.clearDisplay();

  Serial.println("Sprout animation test start.");
}


// ========== 6. loop：反复运行，形成动画 ==========

void loop() {
  /*
    这里我们先用时间自动切换状态，方便测试动画。

    前 5 秒：普通呼吸
    5-10 秒：缺光低头
    10-15 秒：晒太阳开心
    15-20 秒：走路摇摆
    然后循环。
  */

  // millis() 是 Arduino 从开机到现在经过的毫秒数。
  // 1000 毫秒 = 1 秒。
  unsigned long now = millis();

  // 每 20 秒循环一次状态
  int section = (now / 5000) % 4;

  int mood = section;

  // 画当前状态的小芽
  drawSprout(mood);

  // frame 增加，制造下一帧动画
  frame++;

  // 控制动画速度。
  // 100ms 刷新一次，大约一秒 10 帧。
  delay(100);
}