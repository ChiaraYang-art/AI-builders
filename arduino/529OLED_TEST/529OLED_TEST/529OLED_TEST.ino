#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <U8g2lib.h>

// ======================================================
// City Sprout OLED SH1107 标题感干净界面测试版
//
// 适用：M5Stack Unit OLED 1.3"
// 参数：128x64, SH1107, I2C 0x3C
//
// 特点：
// 1. 使用 U8g2 驱动 SH1107
// 2. 不使用反白标题栏，避免左侧黑线
// 3. 保留标题感：标题 + 小装饰点 + 分割线
// 4. 英文显示，避免中文乱码
// ======================================================


// ==================== I2C / PaHUB 设置 ====================

#define SDA_PIN 2
#define SCL_PIN 1

#define PAHUB_ADDR 0x70

// 如果 OLED 直接插在 AtomS3R 的 Grove 口，改成 0
// 如果 OLED 插在 PaHUB 上，保持 1
#define USE_PAHUB 1

// OLED 当前接在 PaHUB channel 1
#define PAHUB_CHANNEL_OLED 1

// OLED I2C 地址
#define OLED_ADDR 0x3C


// ==================== OLED 对象 ====================
//
// 这个 OLED 是 SH1107 128x64。
// U8g2 里用 64x128 驱动 + R1 旋转成横向。
//
U8G2_SH1107_64X128_F_HW_I2C oled(
  U8G2_R1,
  U8X8_PIN_NONE,
  SCL_PIN,
  SDA_PIN
);


// ==================== 页面切换 ====================

unsigned long lastPageTime = 0;
int pageIndex = 0;

const unsigned long PAGE_INTERVAL_MS = 1800;


// ======================================================
// PaHUB 选择通道
// ======================================================

bool selectPaHubChannel(uint8_t channel) {
#if USE_PAHUB
  if (channel > 5) return false;

  Wire.beginTransmission(PAHUB_ADDR);
  Wire.write(1 << channel);
  byte error = Wire.endTransmission();

  delayMicroseconds(800);
  return error == 0;
#else
  return true;
#endif
}


// ======================================================
// OLED 深度清屏
// ======================================================

void deepClearOLED() {
  selectPaHubChannel(PAHUB_CHANNEL_OLED);

  for (int i = 0; i < 4; i++) {
    oled.clearDisplay();
    delay(20);

    oled.clearBuffer();
    oled.sendBuffer();
    delay(20);
  }
}


// ======================================================
// 绘制带标题感的干净页面
// ======================================================

void drawOLEDPage(const char* title, const char* line1, const char* line2, const char* line3) {
  selectPaHubChannel(PAHUB_CHANNEL_OLED);

  oled.clearBuffer();

  oled.setDrawColor(1);
  oled.setFontMode(1);
  oled.setFontDirection(0);

  // --------------------------------------------------
  // 标题区
  // 不使用反白大色块，避免 SH1107 左侧边缘黑线问题
  // --------------------------------------------------

  oled.setFont(u8g2_font_6x10_tf);

  // 标题文字
  oled.drawStr(8, 10, title);

  // 右侧小装饰点，保留一点标题感
  oled.drawDisc(118, 6, 2);

  // 标题下方分割线
  oled.drawLine(8, 16, 120, 16);

  // --------------------------------------------------
  // 主内容区
  // --------------------------------------------------

  oled.setFont(u8g2_font_6x10_tf);

  oled.drawStr(8, 30, line1);
  oled.drawStr(8, 44, line2);
  oled.drawStr(8, 58, line3);

  oled.sendBuffer();
}


// ======================================================
// setup
// ======================================================

void setup() {
  Serial.begin(115200);
  delay(800);

  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Display.setBrightness(120);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(8, 56);
  M5.Display.println("OLED TITLE UI");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  selectPaHubChannel(PAHUB_CHANNEL_OLED);

  oled.setI2CAddress(OLED_ADDR << 1);
  oled.begin();
  oled.setBusClock(100000);
  oled.setPowerSave(0);
  oled.setContrast(180);

  deepClearOLED();

  drawOLEDPage("City Sprout", "OLED Ready", "Title UI", "No black line");

  Serial.println("OLED title UI test started.");
}


// ======================================================
// loop
// ======================================================

void loop() {
  M5.update();

  unsigned long now = millis();

  if (now - lastPageTime >= PAGE_INTERVAL_MS) {
    lastPageTime = now;

    pageIndex++;
    if (pageIndex > 4) pageIndex = 0;

    if (pageIndex == 0) {
      drawOLEDPage("City Sprout", "OLED Ready", "Title UI", "No black line");
    }
    else if (pageIndex == 1) {
      drawOLEDPage("Waiting", "Need light", "Take me out", "Indoor mode");
    }
    else if (pageIndex == 2) {
      drawOLEDPage("Sunlight", "Growing taller", "Blooming", "Light +++");
    }
    else if (pageIndex == 3) {
      drawOLEDPage("Walking", "Exploring", "Soft sway", "Outside world");
    }
    else if (pageIndex == 4) {
      drawOLEDPage("Whoa!", ">   <", "Seeds flying", "Shake detected");
    }
  }
}