#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <U8g2lib.h>

// City Sprout OLED standalone test
// Hardware from current main code:
// I2C: SDA = 2, SCL = 1
// PaHUB: 0x70
// OLED: SH1107 64x128 on PaHUB channel 1

#define SDA_PIN 2
#define SCL_PIN 1
#define PAHUB_ADDR 0x70
#define PAHUB_CHANNEL_OLED 1

U8G2_SH1107_64X128_F_HW_I2C oled(
  U8G2_R0,
  U8X8_PIN_NONE,
  SCL_PIN,
  SDA_PIN
);

enum DemoState {
  DEMO_READY,
  DEMO_LOW_LIGHT,
  DEMO_NEED_LIGHT,
  DEMO_WALKING,
  DEMO_SUNLIGHT,
  DEMO_CITY_SOUND
};

DemoState currentState = DEMO_READY;
bool autoCycle = true;
unsigned long lastCycleTime = 0;
const unsigned long CYCLE_MS = 2500;

static char oledCacheLine0[22] = "";
static char oledCacheLine1[22] = "";
static char oledCacheSpeech0[22] = "";
static char oledCacheSpeech1[22] = "";
static bool oledCacheValid = false;

bool paHubOk = false;
bool oledReady = false;

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

String stateTitle(DemoState state) {
  switch (state) {
    case DEMO_LOW_LIGHT: return "LOW LIGHT";
    case DEMO_NEED_LIGHT: return "NEED LIGHT";
    case DEMO_WALKING: return "WALKING";
    case DEMO_SUNLIGHT: return "SUNLIGHT";
    case DEMO_CITY_SOUND: return "CITY HUM";
    default: return "READY";
  }
}

void stateSpeech(DemoState state, String lines[]) {
  switch (state) {
    case DEMO_LOW_LIGHT:
      lines[0] = "Resting in";
      lines[1] = "soft shade";
      break;
    case DEMO_NEED_LIGHT:
      lines[0] = "Looking for";
      lines[1] = "gentle light";
      break;
    case DEMO_WALKING:
      lines[0] = "Tiny walk";
      lines[1] = "in the city";
      break;
    case DEMO_SUNLIGHT:
      lines[0] = "Warm light";
      lines[1] = "feels good";
      break;
    case DEMO_CITY_SOUND:
      lines[0] = "The street";
      lines[1] = "is humming";
      break;
    default:
      lines[0] = "Quietly";
      lines[1] = "growing";
      break;
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

void drawPlantIcon() {
  // Stem
  oled.drawLine(32, 31, 32, 43);

  // Leaves
  oled.drawDisc(27, 35, 3);
  oled.drawDisc(37, 35, 3);
  oled.drawDisc(32, 29, 2);

  // Tiny roots
  oled.drawLine(32, 43, 28, 47);
  oled.drawLine(32, 43, 36, 47);
}

int activeDotCount(DemoState state) {
  switch (state) {
    case DEMO_LOW_LIGHT: return 1;
    case DEMO_NEED_LIGHT: return 2;
    case DEMO_WALKING: return 3;
    case DEMO_SUNLIGHT: return 4;
    case DEMO_CITY_SOUND: return 3;
    default: return 1;
  }
}

void drawSproutOLED(bool forceUpdate = false) {
  if (paHubOk) selectPaHubChannel(PAHUB_CHANNEL_OLED);

  String line0 = "SPROUT";
  String line1 = stateTitle(currentState);
  String speechLines[2];
  stateSpeech(currentState, speechLines);

  oled.setFont(u8g2_font_6x10_tf);
  line0 = fitOLEDText(line0, 48);
  line1 = fitOLEDText(line1, 58);

  oled.setFont(u8g2_font_5x8_tf);
  String speech0 = fitOLEDText(speechLines[0], 54);
  String speech1 = fitOLEDText(speechLines[1], 54);

  if (!forceUpdate && oledCacheValid &&
      line0 == oledCacheLine0 &&
      line1 == oledCacheLine1 &&
      speech0 == oledCacheSpeech0 &&
      speech1 == oledCacheSpeech1) {
    return;
  }

  oled.clearBuffer();
  oled.setFontMode(1);
  oled.setFontDirection(0);
  oled.setDrawColor(1);

  // Keep all drawing away from x = 0 to avoid edge artifacts.
  oled.drawRFrame(2, 2, 60, 124, 7);

  // Header pill
  oled.drawRBox(8, 7, 48, 15, 5);
  oled.setDrawColor(0);
  oled.setFont(u8g2_font_6x10_tf);
  drawCenteredOLEDText(18, line0.c_str());
  oled.setDrawColor(1);

  drawPlantIcon();

  // State title
  oled.setFont(u8g2_font_6x10_tf);
  drawCenteredOLEDText(58, line1.c_str());

  // Message card
  oled.drawRFrame(7, 68, 50, 35, 5);
  oled.setFont(u8g2_font_5x8_tf);
  drawCenteredOLEDText(82, speech0.c_str());
  drawCenteredOLEDText(96, speech1.c_str());

  // Bottom dots
  int activeDots = activeDotCount(currentState);
  for (int i = 0; i < 4; i++) {
    int x = 20 + i * 8;
    if (i < activeDots) oled.drawDisc(x, 116, 2);
    else oled.drawCircle(x, 116, 2);
  }

  oled.sendBuffer();

  line0.toCharArray(oledCacheLine0, sizeof(oledCacheLine0));
  line1.toCharArray(oledCacheLine1, sizeof(oledCacheLine1));
  speech0.toCharArray(oledCacheSpeech0, sizeof(oledCacheSpeech0));
  speech1.toCharArray(oledCacheSpeech1, sizeof(oledCacheSpeech1));
  oledCacheValid = true;
}

void showMainScreen() {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setTextSize(1);
  M5.Display.drawString("OLED TEST", 64, 22);
  M5.Display.drawString(stateTitle(currentState), 64, 50);
  M5.Display.setTextDatum(top_left);
  M5.Display.drawString("Serial: 115200", 8, 78);
  M5.Display.drawString("demo / sun / walk", 8, 94);
  M5.Display.drawString(autoCycle ? "mode: auto" : "mode: manual", 8, 110);
}

void printHelp() {
  Serial.println();
  Serial.println("===== City Sprout OLED Test =====");
  Serial.println("Commands:");
  Serial.println("  demo / auto  : auto cycle states");
  Serial.println("  stop         : stop auto cycle");
  Serial.println("  idle         : READY");
  Serial.println("  dark         : LOW LIGHT");
  Serial.println("  need         : NEED LIGHT");
  Serial.println("  walk         : WALKING");
  Serial.println("  sun          : SUNLIGHT");
  Serial.println("  city         : CITY HUM");
  Serial.println("  help         : print commands");
  Serial.println("=================================");
  Serial.println();
}

void setDemoState(DemoState state, bool manual = true) {
  currentState = state;
  oledCacheValid = false;
  drawSproutOLED(true);
  showMainScreen();

  Serial.print("OLED state -> ");
  Serial.println(stateTitle(currentState));

  if (manual) autoCycle = false;
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
    Serial.println("Auto demo mode ON.");
  } else if (cmd == "stop" || cmd == "manual") {
    autoCycle = false;
    Serial.println("Auto demo mode OFF.");
  } else if (cmd == "idle" || cmd == "ready") {
    setDemoState(DEMO_READY);
  } else if (cmd == "dark" || cmd == "low") {
    setDemoState(DEMO_LOW_LIGHT);
  } else if (cmd == "need" || cmd == "light") {
    setDemoState(DEMO_NEED_LIGHT);
  } else if (cmd == "walk" || cmd == "walking") {
    setDemoState(DEMO_WALKING);
  } else if (cmd == "sun" || cmd == "sunlight") {
    setDemoState(DEMO_SUNLIGHT);
  } else if (cmd == "city" || cmd == "sound") {
    setDemoState(DEMO_CITY_SOUND);
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
    case DEMO_READY: currentState = DEMO_LOW_LIGHT; break;
    case DEMO_LOW_LIGHT: currentState = DEMO_NEED_LIGHT; break;
    case DEMO_NEED_LIGHT: currentState = DEMO_WALKING; break;
    case DEMO_WALKING: currentState = DEMO_SUNLIGHT; break;
    case DEMO_SUNLIGHT: currentState = DEMO_CITY_SOUND; break;
    default: currentState = DEMO_READY; break;
  }

  oledCacheValid = false;
  drawSproutOLED(true);
  showMainScreen();

  Serial.print("Auto state -> ");
  Serial.println(stateTitle(currentState));
}

void setup() {
  Serial.begin(115200);
  delay(500);

  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(120);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.drawString("OLED TEST BOOT", 64, 46);
  M5.Display.drawString("starting...", 64, 70);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  scanCurrentI2C("Before PaHUB select");

  paHubOk = selectPaHubChannel(PAHUB_CHANNEL_OLED);
  Serial.print("PaHUB select channel 1: ");
  Serial.println(paHubOk ? "OK" : "FAILED / not found");

  scanCurrentI2C("OLED channel");

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

  oledReady = true;
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
