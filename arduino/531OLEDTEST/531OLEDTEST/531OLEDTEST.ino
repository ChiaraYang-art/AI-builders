#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <U8g2lib.h>

// =====================================================
// City Sprout OLED 可爱挂件风 - 最终单独测试版
// 去掉 LIGHT / MOVE / SOUND 标签
//
// 只测试：PAHUB + SH1107 OLED
// 不需要 arduino_secrets.h
// 不需要 WiFi / TTS / 麦克风 / ENV
//
// 状态逻辑对应最终方案：
// 感光：光偏暗 / 想找光 / 晒太阳
// 感移动：散步中
// 感噪音：听城市 / 声音近
// 基础：待机中
// =====================================================


// ===============================
// I2C / PAHUB / OLED 设置
// ===============================

#define SDA_PIN 2
#define SCL_PIN 1

#define PAHUB_ADDR 0x70
#define PAHUB_CHANNEL_OLED 1

// SH1107 OLED 64x128
U8G2_SH1107_64X128_F_HW_I2C oled(
  U8G2_R0,
  U8X8_PIN_NONE,
  SCL_PIN,
  SDA_PIN
);


// ===============================
// Demo 状态
// ===============================

enum OLEDTestState {
  TEST_IDLE,
  TEST_LOW_LIGHT,
  TEST_NEED_LIGHT,
  TEST_SUNLIGHT,
  TEST_WALKING,
  TEST_CITY_SOUND,
  TEST_LOUD_SOUND
};

OLEDTestState currentState = TEST_IDLE;

bool autoCycle = true;
bool paHubOk = false;

unsigned long lastCycleTime = 0;
const unsigned long CYCLE_MS = 2600;


// ===============================
// OLED 缓存，减少闪烁
// ===============================

static char oledCacheTitle[32] = "";
static char oledCacheLine0[32] = "";
static char oledCacheLine1[32] = "";
static bool oledCacheValid = false;


// ===============================
// PAHUB / I2C 工具
// ===============================

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

void scanCurrentI2C(const char* label) {
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

  if (!foundAny) Serial.print(" no device");
  Serial.println();
}


// ===============================
// 字体工具
// ===============================

void setCNFont() {
  // 如果这一行编译报字体不存在，
  // 就把它改成：oled.setFont(u8g2_font_unifont_t_chinese2);
  oled.setFont(u8g2_font_wqy12_t_gb2312);
}

void setAsciiTitleFont() {
  oled.setFont(u8g2_font_6x10_tf);
}

void drawCenteredASCII(int y, const char* text) {
  int w = oled.getStrWidth(text);
  int x = (64 - w) / 2;
  if (x < 2) x = 2;
  oled.drawStr(x, y, text);
}

void drawCenteredUTF8(int y, const String& text) {
  int w = oled.getUTF8Width(text.c_str());
  int x = (64 - w) / 2;
  if (x < 2) x = 2;
  oled.drawUTF8(x, y, text.c_str());
}


// ===============================
// 状态文案
// ===============================

String getStateTitle(OLEDTestState state) {
  switch (state) {
    case TEST_LOW_LIGHT:
      return "光偏暗";

    case TEST_NEED_LIGHT:
      return "想找光";

    case TEST_SUNLIGHT:
      return "晒太阳";

    case TEST_WALKING:
      return "散步中";

    case TEST_CITY_SOUND:
      return "听城市";

    case TEST_LOUD_SOUND:
      return "声音近";

    default:
      return "待机中";
  }
}

void getSpeechLines(OLEDTestState state, String lines[]) {
  switch (state) {
    case TEST_LOW_LIGHT:
      lines[0] = "先休息";
      lines[1] = "等点光";
      break;

    case TEST_NEED_LIGHT:
      lines[0] = "去窗边";
      lines[1] = "找软光";
      break;

    case TEST_SUNLIGHT:
      lines[0] = "光很软";
      lines[1] = "慢慢长";
      break;

    case TEST_WALKING:
      lines[0] = "被带着";
      lines[1] = "看城市";
      break;

    case TEST_CITY_SOUND:
      lines[0] = "街声轻";
      lines[1] = "叶子听";
      break;

    case TEST_LOUD_SOUND:
      lines[0] = "有点响";
      lines[1] = "叶子抖";
      break;

    default:
      lines[0] = "安静地";
      lines[1] = "发小芽";
      break;
  }
}

int getStateLevel(OLEDTestState state) {
  switch (state) {
    case TEST_LOW_LIGHT:
      return 1;

    case TEST_NEED_LIGHT:
      return 2;

    case TEST_LOUD_SOUND:
      return 2;

    case TEST_WALKING:
      return 3;

    case TEST_CITY_SOUND:
      return 3;

    case TEST_SUNLIGHT:
      return 4;

    default:
      return 1;
  }
}


// ===============================
// 可爱挂件风装饰元素
// ===============================

void drawTinySparkle(int x, int y) {
  oled.drawPixel(x, y - 2);
  oled.drawPixel(x, y + 2);
  oled.drawPixel(x - 2, y);
  oled.drawPixel(x + 2, y);
  oled.drawPixel(x, y);
}

void drawTinyLeaf(int x, int y, bool flip) {
  if (!flip) {
    oled.drawPixel(x, y);
    oled.drawPixel(x + 1, y - 1);
    oled.drawPixel(x + 2, y - 1);
    oled.drawPixel(x + 1, y);
    oled.drawPixel(x + 2, y);
  } else {
    oled.drawPixel(x, y);
    oled.drawPixel(x - 1, y - 1);
    oled.drawPixel(x - 2, y - 1);
    oled.drawPixel(x - 1, y);
    oled.drawPixel(x - 2, y);
  }
}

void drawTopDecor() {
  // 顶部装饰：小点 + 小星 + 小叶子
  oled.drawDisc(11, 31, 1);
  oled.drawDisc(18, 31, 1);

  drawTinySparkle(32, 31);

  oled.drawDisc(46, 31, 1);
  oled.drawDisc(53, 31, 1);

  drawTinyLeaf(24, 34, false);
  drawTinyLeaf(40, 34, true);
}

void drawSoftSpeechBubble() {
  // 中下部圆角对话框
  oled.drawRFrame(8, 65, 48, 35, 6);

  // 小尾巴，让它更像“会说话的小挂件”
  oled.drawLine(28, 100, 31, 104);
  oled.drawLine(31, 104, 35, 100);

  // 轻装饰点
  oled.drawPixel(13, 70);
  oled.drawPixel(51, 94);
}

void drawStateMiniIcon(OLEDTestState state) {
  int cx = 32;
  int cy = 55;

  if (state == TEST_SUNLIGHT) {
    // 小太阳
    oled.drawCircle(cx, cy, 2);
    oled.drawPixel(cx, cy - 5);
    oled.drawPixel(cx, cy + 5);
    oled.drawPixel(cx - 5, cy);
    oled.drawPixel(cx + 5, cy);
    oled.drawPixel(cx - 3, cy - 3);
    oled.drawPixel(cx + 3, cy - 3);
    oled.drawPixel(cx - 3, cy + 3);
    oled.drawPixel(cx + 3, cy + 3);

  } else if (state == TEST_WALKING) {
    // 小脚步
    oled.drawDisc(cx - 3, cy, 1);
    oled.drawDisc(cx + 3, cy - 2, 1);
    oled.drawDisc(cx - 1, cy + 4, 1);

  } else if (state == TEST_LOW_LIGHT || state == TEST_NEED_LIGHT) {
    // 小月牙
    oled.drawCircle(cx, cy, 3);
    oled.setDrawColor(0);
    oled.drawDisc(cx + 2, cy - 1, 3);
    oled.setDrawColor(1);

  } else if (state == TEST_CITY_SOUND || state == TEST_LOUD_SOUND) {
    // 小声波
    oled.drawLine(cx - 5, cy, cx - 5, cy + 3);
    oled.drawLine(cx, cy - 3, cx, cy + 3);
    oled.drawLine(cx + 5, cy - 1, cx + 5, cy + 3);

  } else {
    // 小芽
    oled.drawLine(cx, cy - 2, cx, cy + 4);
    drawTinyLeaf(cx - 1, cy, true);
    drawTinyLeaf(cx + 1, cy - 1, false);
  }
}

void drawBottomCharm(OLEDTestState state) {
  int active = getStateLevel(state);

  // 底部细线，像挂件底部装饰边
  oled.drawLine(12, 109, 52, 109);

  // 中间小芽 / 小花
  oled.drawLine(32, 113, 32, 119);
  drawTinyLeaf(31, 116, true);
  drawTinyLeaf(33, 115, false);
  oled.drawDisc(32, 112, 1);

  // 四个状态点，左右分布
  int xs[4] = {17, 25, 39, 47};

  for (int i = 0; i < 4; i++) {
    if (i < active) {
      oled.drawDisc(xs[i], 120, 1);
    } else {
      oled.drawCircle(xs[i], 120, 1);
    }
  }
}


// ===============================
// OLED 主界面
// ===============================

void drawSproutOLED(bool forceUpdate = false) {
  if (paHubOk) {
    selectPaHubChannel(PAHUB_CHANNEL_OLED);
  }

  String title = getStateTitle(currentState);

  String speechLines[2];
  getSpeechLines(currentState, speechLines);

  String line0 = speechLines[0];
  String line1 = speechLines[1];

  if (!forceUpdate && oledCacheValid &&
      title == oledCacheTitle &&
      line0 == oledCacheLine0 &&
      line1 == oledCacheLine1) {
    return;
  }

  oled.clearBuffer();
  oled.setFontMode(1);
  oled.setFontDirection(0);
  oled.setDrawColor(1);

  // 一层外框：像挂件边框，避开 x=0 防止左侧黑线
  oled.drawRFrame(2, 2, 60, 124, 8);

  // 顶部 SPROUT 胶囊
  oled.drawRBox(9, 8, 46, 14, 5);

  oled.setDrawColor(0);
  setAsciiTitleFont();
  drawCenteredASCII(19, "SPROUT");
  oled.setDrawColor(1);

  // 顶部装饰带
  drawTopDecor();

  // 主状态标题
  setCNFont();
  drawCenteredUTF8(48, title);

  // 状态小图标
  drawStateMiniIcon(currentState);

  // 对话框
  drawSoftSpeechBubble();

  // 对话框内文案
  setCNFont();
  drawCenteredUTF8(82, line0);
  drawCenteredUTF8(96, line1);

  // 底部挂件装饰 + 状态点
  drawBottomCharm(currentState);

  oled.sendBuffer();

  title.toCharArray(oledCacheTitle, sizeof(oledCacheTitle));
  line0.toCharArray(oledCacheLine0, sizeof(oledCacheLine0));
  line1.toCharArray(oledCacheLine1, sizeof(oledCacheLine1));
  oledCacheValid = true;
}


// ===============================
// M5 主屏辅助显示
// ===============================

void showMainScreen() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(1);

  M5.Display.setCursor(10, 16);
  M5.Display.print("OLED CUTE FINAL");

  M5.Display.setCursor(10, 40);
  M5.Display.print("PAHUB CH1");

  M5.Display.setCursor(10, 64);
  M5.Display.print("Serial: 115200");

  M5.Display.setCursor(10, 88);
  M5.Display.print(autoCycle ? "mode: auto" : "mode: manual");

  M5.Display.setCursor(10, 108);
  M5.Display.print(getStateTitle(currentState));
}


// ===============================
// 串口命令
// ===============================

void printHelp() {
  Serial.println();
  Serial.println("===== City Sprout OLED Cute Charm Final Test =====");
  Serial.println("Commands:");
  Serial.println("  demo / auto  : 自动轮播");
  Serial.println("  stop         : 停止轮播");
  Serial.println("  idle         : 待机中");
  Serial.println("  dark         : 光偏暗");
  Serial.println("  need         : 想找光");
  Serial.println("  sun          : 晒太阳");
  Serial.println("  walk         : 散步中");
  Serial.println("  city         : 听城市");
  Serial.println("  loud         : 声音近");
  Serial.println("  help         : 显示命令");
  Serial.println("=================================================");
  Serial.println();
}

void setDemoState(OLEDTestState state, bool manual = true) {
  currentState = state;
  oledCacheValid = false;

  if (manual) {
    autoCycle = false;
  }

  drawSproutOLED(true);
  showMainScreen();

  Serial.print("OLED state -> ");
  Serial.println(getStateTitle(currentState));
}

void handleSerialCommand() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();

  if (cmd.length() == 0) return;

  if (cmd == "demo" || cmd == "auto") {
    autoCycle = true;
    lastCycleTime = 0;
    oledCacheValid = false;
    Serial.println("Auto demo mode ON.");
    showMainScreen();

  } else if (cmd == "stop" || cmd == "manual") {
    autoCycle = false;
    Serial.println("Auto demo mode OFF.");
    showMainScreen();

  } else if (cmd == "idle" || cmd == "ready") {
    setDemoState(TEST_IDLE);

  } else if (cmd == "dark" || cmd == "low") {
    setDemoState(TEST_LOW_LIGHT);

  } else if (cmd == "need" || cmd == "light") {
    setDemoState(TEST_NEED_LIGHT);

  } else if (cmd == "sun" || cmd == "sunlight") {
    setDemoState(TEST_SUNLIGHT);

  } else if (cmd == "walk" || cmd == "walking" || cmd == "move") {
    setDemoState(TEST_WALKING);

  } else if (cmd == "city" || cmd == "sound") {
    setDemoState(TEST_CITY_SOUND);

  } else if (cmd == "loud" || cmd == "intense" || cmd == "noise") {
    setDemoState(TEST_LOUD_SOUND);

  } else if (cmd == "help") {
    printHelp();

  } else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
    printHelp();
  }
}

void nextAutoState() {
  switch (currentState) {
    case TEST_IDLE:
      currentState = TEST_LOW_LIGHT;
      break;

    case TEST_LOW_LIGHT:
      currentState = TEST_NEED_LIGHT;
      break;

    case TEST_NEED_LIGHT:
      currentState = TEST_SUNLIGHT;
      break;

    case TEST_SUNLIGHT:
      currentState = TEST_WALKING;
      break;

    case TEST_WALKING:
      currentState = TEST_CITY_SOUND;
      break;

    case TEST_CITY_SOUND:
      currentState = TEST_LOUD_SOUND;
      break;

    default:
      currentState = TEST_IDLE;
      break;
  }

  oledCacheValid = false;
  drawSproutOLED(true);
  showMainScreen();

  Serial.print("Auto state -> ");
  Serial.println(getStateTitle(currentState));
}


// ===============================
// setup / loop
// ===============================

void setup() {
  Serial.begin(115200);
  delay(500);

  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Display.setRotation(0);
  M5.Display.setBrightness(120);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(1);

  M5.Display.setCursor(10, 38);
  M5.Display.print("OLED CUTE FINAL");

  M5.Display.setCursor(10, 60);
  M5.Display.print("starting...");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  scanCurrentI2C("Before PaHUB");

  paHubOk = selectPaHubChannel(PAHUB_CHANNEL_OLED);

  Serial.print("PaHUB select channel 1: ");
  Serial.println(paHubOk ? "OK" : "FAILED / not found");

  scanCurrentI2C("OLED channel");

  oled.begin();
  oled.enableUTF8Print();
  oled.setBusClock(100000);
  oled.setContrast(180);
  oled.setFontMode(1);
  oled.setFontDirection(0);
  oled.setDrawColor(1);

  oled.clearBuffer();
  oled.sendBuffer();
  delay(50);

  drawSproutOLED(true);
  showMainScreen();
  printHelp();
}

void loop() {
  M5.update();

  handleSerialCommand();

  if (autoCycle && millis() - lastCycleTime >= CYCLE_MS) {
    lastCycleTime = millis();
    nextAutoState();
  }
}