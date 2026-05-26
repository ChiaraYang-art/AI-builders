/*
  出走小芽 Demo - 只有 DLight + AI 后端，不接 OLED

  适合现在没有 PaHUB / Unit Hub 的情况。

  功能：
  1. 读取光照
  2. 发给电脑后端
  3. 把后端返回的植物语言打印到 Serial Monitor
*/


#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>


const char* WIFI_SSID = "DAElab";
const char* WIFI_PASSWORD = "tjdaelab";
const char* SERVER_URL = "http://192.168.10.36:5000/plant";


#define SDA_PIN 2
#define SCL_PIN 1
#define BH1750_ADDR 0x23


void bh1750WriteCommand(byte command) {
  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(command);
  Wire.endTransmission();
}


float readLightLux() {
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


void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("AtomS3R IP: ");
  Serial.println(WiFi.localIP());
}


String askPlantServer(float lux) {
  if (WiFi.status() != WL_CONNECTED) {
    return "WiFi not connected";
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"lux\":";
  payload += String(lux, 2);
  payload += "}";

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    String response = http.getString();
    http.end();
    return response;
  } else {
    http.end();
    return "AI server error";
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("DLight + AI text demo starting...");

  Wire.begin(SDA_PIN, SCL_PIN);

  bh1750WriteCommand(0x01);
  delay(10);
  bh1750WriteCommand(0x10);
  delay(200);

  connectWiFi();
}


void loop() {
  float lux = readLightLux();

  if (lux < 0) {
    Serial.println("Failed to read light sensor.");
    delay(2000);
    return;
  }

  Serial.print("Lux: ");
  Serial.println(lux);

  String sentence = askPlantServer(lux);

  Serial.print("Plant says: ");
  Serial.println(sentence);

  Serial.println("-------------------");

  delay(8000);
}