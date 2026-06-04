#pragma once

// Copy this file to arduino_secrets.h in the same folder.
// Fill in your real values locally.
// Do not commit arduino_secrets.h to GitHub.

#define SECRET_WIFI_SSID "your-wifi-name"
#define SECRET_WIFI_PASSWORD "your-wifi-password"
#define SECRET_SERVER_URL "http://your-server-ip:5000/plant"

// 可选：若不填，主程序会从 SECRET_SERVER_URL 自动推导
// #define SECRET_AUDIO_URL "http://your-server-ip:5000/audio/latest.mp3"
