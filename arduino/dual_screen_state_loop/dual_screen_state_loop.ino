/*
  City Sprout / 出走小芽
  文件：dual_screen_state_loop.ino

  这个程序对应“视觉演示模式”：
  1. 只连接外接 OLED，不连接 DLight 光照传感器。
  2. AtomS3R 自带彩屏显示彩色小芽动画。
  3. 外接 OLED 横屏显示植物状态和植物说的话。
  4. 程序每 5 秒自动切换一个状态，方便课堂展示完整体验。

  当前硬件连接：
  AtomS3R Grove / HY2.0-4P
  -> M5Stack Unit OLED 1.3 inch

  重要提醒：
  - 外接 OLED 是黑白屏，所以它只负责文字。
  - AtomS3R 自带屏是彩屏，所以它负责小芽动画。
  - 这个模式不需要 Wi-Fi，也不需要 Flask 后端。
*/

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "M5AtomS3.h"

// =========================
// 1. Grove / I2C 引脚设置
// =========================

// SDA_PIN = 2 表示 AtomS3R 的 G2 引脚。
// SDA 是 I2C 的“数据线”，OLED 会通过这根线收发数据。
// 后续接 Unit Hub / PaHUB 时，OLED、DLight、ENV Pro 都会共享这根 SDA 线。
#define SDA_PIN 2

// SCL_PIN = 1 表示 AtomS3R 的 G1 引脚。
// SCL 是 I2C 的“时钟线”，用来同步 AtomS3R 和 OLED 的通信节奏。
// 这两个引脚是之前已经在 OLED 和 DLight 单独测试中跑通的组合。
#define SCL_PIN 1

// =========================
// 2. 外接 OLED 对象
// =========================

// 这块 OLED 的驱动芯片是 SH1107。
// U8g2 库里要用 64X128 这个构造器，因为屏幕原生方向是竖的 64x128。
// U8G2_R1 表示把屏幕旋转 90 度，让它横屏显示成 128x64。
// 如果现场发现文字方向反了，可以把 U8G2_R1 改成 U8G2_R3。
U8G2_SH1107_64X128_F_HW_I2C oled(
  U8G2_R1,
  U8X8_PIN_NONE,
  SCL_PIN,
  SDA_PIN
);

// =========================
// 3. 植物状态定义
// =========================

// enum 可以理解成“固定选项列表”。
// 这里统一使用交接文档要求的 5 个状态名，后续 sensor / backend / main 程序都沿用这套名字。
enum PlantState {
  STATE_IDLE = 0,      // 待机：小芽安静等待。
  STATE_WILTED = 1,    // 蔫了：长期低光照，看起来低头、叶子暗。
  STATE_NEED_SUN = 2,  // 想晒太阳：小芽主动邀请用户带它去真实阳光下。
  STATE_SUNLIGHT = 3,  // 得到阳光：光照足够，小芽展开叶子。
  STATE_WALKING = 4    // 被带着走：后续由 IMU 检测移动，这里先用轮播模拟。
};

// 每个状态停留 5 秒。
// 5000 毫秒 = 5 秒。用常量写出来，后续想改节奏只改这一行。
const unsigned long STATE_DURATION_MS = 5000;

// S3R 彩屏动画刷新间隔。
// 120ms 大约是一秒 8 帧，足够让小芽看起来在动，同时不会刷新太快。
const unsigned long FRAME_INTERVAL_MS = 120;

// frame 用来记录动画第几帧。
// 它会不断增加，我们用它计算“呼吸”“摆动”等周期变化。
int frame = 0;

// 记录上一次刷新 S3R 彩屏的时间。
// 这样 loop 里不会疯狂刷新屏幕，而是按 FRAME_INTERVAL_MS 稳定刷新。
unsigned long lastFrameTime = 0;

// 记录 OLED 当前显示的状态。
// OLED 只需要在状态变化时重画，不需要每一帧动画都刷新。
PlantState lastOledState = (PlantState)(-1);

// =========================
// 4. 状态文本和本地文案
// =========================

String plantStateToText(PlantState state) {
  /*
    作用：
    把程序内部的状态枚举转换成后端/API 也能理解的英文小写文本。

    输入：
    state：当前植物状态，例如 STATE_WILTED。

    输出：
    "wilted"、"need_sun" 等短字符串。
  */
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

String plantStateToTitle(PlantState state) {
  /*
    作用：
    给 OLED 顶部显示一个人类容易读的标题。

    为什么不用中文：
    这块 OLED 很小，中文需要额外字库，第一版用英文更稳定。
  */
  if (state == STATE_IDLE) {
    return "Idle";
  }

  if (state == STATE_WILTED) {
    return "Wilted";
  }

  if (state == STATE_NEED_SUN) {
    return "Need Sun";
  }

  if (state == STATE_SUNLIGHT) {
    return "Sunlight";
  }

  if (state == STATE_WALKING) {
    return "Walking";
  }

  return "Idle";
}

String getLocalPlantSpeech(PlantState state) {
  /*
    作用：
    根据状态返回一句固定植物语言。

    为什么需要本地文案：
    第一版视觉 Demo 不依赖网络和后端。
    后续接 Flask / 大模型时，如果后端请求失败，也可以退回到这里的固定文案。
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

PlantState getAutoLoopState() {
  /*
    作用：
    不用传感器，直接按时间自动轮播 5 个状态。

    millis() 是 Arduino 从开机到现在经过的毫秒数。
    例如开机 13000ms 后：
    13000 / 5000 = 2
    2 % 5 = 2
    所以当前显示第 2 个状态，也就是 STATE_NEED_SUN。
  */
  int index = (millis() / STATE_DURATION_MS) % 5;

  if (index == 0) {
    return STATE_IDLE;
  }

  if (index == 1) {
    return STATE_WILTED;
  }

  if (index == 2) {
    return STATE_NEED_SUN;
  }

  if (index == 3) {
    return STATE_SUNLIGHT;
  }

  return STATE_WALKING;
}

// =========================
// 5. OLED 自动换行
// =========================

void wrapTextForOLED(String text, String lines[], int maxLines) {
  /*
    作用：
    把一句英文短句拆成多行，避免超出 OLED 宽度。

    输入：
    text：要显示的完整句子。
    lines：输出数组，函数会把每一行文字放进去。
    maxLines：最多允许几行。

    为什么按字符数换行：
    这里使用 u8g2_font_6x10_tr 字体，每个英文字符大约 6 像素宽。
    OLED 横屏宽度是 128 像素，左右留边后每行大约放 18 个字符比较稳。
  */
  const int MAX_CHARS_PER_LINE = 18;

  for (int i = 0; i < maxLines; i++) {
    lines[i] = "";
  }

  int currentLine = 0;
  String currentWord = "";

  for (int i = 0; i <= text.length(); i++) {
    char c = (i < text.length()) ? text.charAt(i) : ' ';

    if (c == ' ') {
      if (currentWord.length() == 0) {
        continue;
      }

      String candidate = lines[currentLine];
      if (candidate.length() > 0) {
        candidate += " ";
      }
      candidate += currentWord;

      if (candidate.length() <= MAX_CHARS_PER_LINE) {
        lines[currentLine] = candidate;
      } else {
        currentLine++;
        if (currentLine >= maxLines) {
          return;
        }
        lines[currentLine] = currentWord;
      }

      currentWord = "";
    } else {
      currentWord += c;
    }
  }
}

void showTextOnOLED(PlantState state, String speech) {
  /*
    作用：
    在外接黑白 OLED 上显示状态标题和植物说的话。

    OLED 坐标说明：
    - x 表示从左往右的位置，0 是最左边，127 是最右边。
    - y 表示从上往下的位置，0 是最上边，63 是最下边。
    - drawStr(x, y, text) 的 y 是文字基线，不是文字框顶部。
  */
  String lines[3];
  wrapTextForOLED(speech, lines, 3);

  oled.clearBuffer();

  // 顶部状态栏使用稍小字体，给正文留空间。
  oled.setFont(u8g2_font_6x10_tr);
  String title = "Status: " + plantStateToTitle(state);
  oled.drawStr(0, 10, title.c_str());

  // 分隔线让“状态”和“植物语言”在小屏上更容易区分。
  oled.drawLine(0, 14, 127, 14);

  // 正文最多 3 行。每行 y 坐标相差 12 像素，避免文字互相压住。
  for (int i = 0; i < 3; i++) {
    if (lines[i].length() > 0) {
      oled.drawStr(0, 29 + i * 12, lines[i].c_str());
    }
  }

  oled.sendBuffer();
}

// =========================
// 6. S3R 彩屏小芽动画
// =========================

uint16_t stateBackgroundColor(PlantState state) {
  /*
    作用：
    根据状态给 AtomS3R 自带彩屏选择背景色。
    注意这里是彩屏颜色，和外接 OLED 无关。
  */
  if (state == STATE_WILTED) {
    return AtomS3.Display.color565(42, 48, 42);
  }

  if (state == STATE_NEED_SUN) {
    return AtomS3.Display.color565(40, 66, 82);
  }

  if (state == STATE_SUNLIGHT) {
    return AtomS3.Display.color565(255, 205, 92);
  }

  if (state == STATE_WALKING) {
    return AtomS3.Display.color565(52, 76, 96);
  }

  return AtomS3.Display.color565(28, 40, 35);
}

void drawSproutOnS3R(PlantState state) {
  /*
    作用：
    在 AtomS3R 自带彩屏上画彩色小芽。

    设计原则：
    - 自带屏只显示小芽姿态和极短状态角标。
    - 不在彩屏上显示长句，长句交给外接 OLED。
  */
  int w = AtomS3.Display.width();
  int h = AtomS3.Display.height();
  int centerX = w / 2;

  // phase 会在 0 到 23 之间循环，用来制造周期动画。
  int phase = frame % 24;

  // breathe 用于“呼吸感”：前半周期上移，后半周期下移。
  int breathe = (phase < 12) ? -2 : 2;

  // sway 用于“左右摆动”：walking 状态会放大这个数值。
  int sway = 0;
  if (phase < 6) {
    sway = -2;
  } else if (phase < 12) {
    sway = -1;
  } else if (phase < 18) {
    sway = 1;
  } else {
    sway = 2;
  }

  int baseY = h - 26;
  int topY = 50 + breathe;
  int leafDrop = 0;
  int stemSway = sway;

  uint16_t stemColor = AtomS3.Display.color565(80, 170, 92);
  uint16_t leafColor = AtomS3.Display.color565(58, 190, 86);
  uint16_t potColor = AtomS3.Display.color565(138, 82, 50);
  uint16_t soilColor = AtomS3.Display.color565(92, 60, 42);

  if (state == STATE_WILTED) {
    topY = 64;
    leafDrop = 10;
    stemSway = -4;
    stemColor = AtomS3.Display.color565(72, 112, 70);
    leafColor = AtomS3.Display.color565(54, 104, 58);
  } else if (state == STATE_NEED_SUN) {
    topY = 52 + breathe;
    leafDrop = 2;
    leafColor = AtomS3.Display.color565(76, 170, 82);
  } else if (state == STATE_SUNLIGHT) {
    topY = 42 + breathe;
    leafDrop = -6;
    stemColor = AtomS3.Display.color565(54, 150, 62);
    leafColor = AtomS3.Display.color565(34, 205, 84);
  } else if (state == STATE_WALKING) {
    topY = 50;
    stemSway = sway * 3;
  }

  AtomS3.Display.fillScreen(stateBackgroundColor(state));

  // sunlight 状态画几个太阳粒子，让“晒到光”的状态一眼能看出来。
  if (state == STATE_SUNLIGHT) {
    uint16_t sunColor = AtomS3.Display.color565(255, 245, 190);
    AtomS3.Display.fillCircle(w - 22, 22, 10, sunColor);
    AtomS3.Display.drawLine(w - 22, 6, w - 22, 0, sunColor);
    AtomS3.Display.drawLine(w - 22, 38, w - 22, 46, sunColor);
    AtomS3.Display.drawLine(w - 38, 22, w - 48, 22, sunColor);
    AtomS3.Display.drawLine(w - 6, 22, w, 22, sunColor);
  }

  // walking 状态画短动线，暗示设备正在被拿着移动。
  if (state == STATE_WALKING) {
    uint16_t motionColor = AtomS3.Display.color565(150, 210, 230);
    AtomS3.Display.drawLine(12, 44, 30, 44, motionColor);
    AtomS3.Display.drawLine(8, 62, 25, 62, motionColor);
    AtomS3.Display.drawLine(w - 30, 54, w - 12, 54, motionColor);
  }

  // 花盆：先画土，再画盆。坐标固定在屏幕底部。
  AtomS3.Display.fillRoundRect(centerX - 27, baseY + 6, 54, 18, 5, potColor);
  AtomS3.Display.fillRoundRect(centerX - 30, baseY, 60, 10, 4, soilColor);

  // 茎：用两条线让它比单线更有厚度。
  AtomS3.Display.drawLine(centerX, baseY, centerX + stemSway, topY, stemColor);
  AtomS3.Display.drawLine(centerX + 1, baseY, centerX + stemSway + 1, topY, stemColor);

  // 两片叶子：用椭圆表示，状态不同会改变叶子下垂程度。
  int leafY = topY + 20;
  AtomS3.Display.fillEllipse(centerX - 16 + stemSway, leafY + leafDrop, 19, 9, leafColor);
  AtomS3.Display.fillEllipse(centerX + 17 + stemSway, leafY + leafDrop - 2, 19, 9, leafColor);

  // 小芽顶部：圆点表示新芽。
  AtomS3.Display.fillCircle(centerX + stemSway, topY, 7, leafColor);

  // 蔫了时显示三个小点，表达“没精神”，但不显示长文本。
  if (state == STATE_WILTED) {
    AtomS3.Display.setTextColor(AtomS3.Display.color565(210, 220, 205));
    AtomS3.Display.setTextSize(1);
    AtomS3.Display.setCursor(centerX - 9, 26);
    AtomS3.Display.print("...");
  }

  // 左上角小状态角标。只放短词，不抢 OLED 的文字职责。
  AtomS3.Display.fillRoundRect(5, 5, 52, 17, 4, AtomS3.Display.color565(0, 0, 0));
  AtomS3.Display.setTextColor(WHITE);
  AtomS3.Display.setTextSize(1);
  AtomS3.Display.setCursor(10, 10);
  AtomS3.Display.print(plantStateToTitle(state));
}

// =========================
// 7. Arduino 标准入口
// =========================

void setup() {
  Serial.begin(115200);
  delay(500);

  // 初始化 AtomS3R 本体和自带彩屏。
  // M5.config() 会读取默认硬件配置，AtomS3.begin(cfg) 会启动屏幕等板载资源。
  auto cfg = M5.config();
  AtomS3.begin(cfg);
  AtomS3.Display.setRotation(0);
  AtomS3.Display.setBrightness(120);

  // 初始化 I2C 总线。OLED 依靠 SDA_PIN / SCL_PIN 这两根线通信。
  Wire.begin(SDA_PIN, SCL_PIN);

  // 初始化外接 OLED。
  oled.begin();
  oled.setContrast(255);
  oled.clearDisplay();

  Serial.println("City Sprout visual demo started.");
}

void loop() {
  PlantState currentState = getAutoLoopState();
  String speech = getLocalPlantSpeech(currentState);

  // OLED 只在状态变化时刷新一次，避免文字闪烁。
  if (currentState != lastOledState) {
    showTextOnOLED(currentState, speech);
    lastOledState = currentState;

    Serial.print("State: ");
    Serial.print(plantStateToText(currentState));
    Serial.print(" | Speech: ");
    Serial.println(speech);
  }

  // S3R 彩屏持续刷新动画。
  unsigned long now = millis();
  if (now - lastFrameTime >= FRAME_INTERVAL_MS) {
    drawSproutOnS3R(currentState);
    frame++;
    lastFrameTime = now;
  }
}
