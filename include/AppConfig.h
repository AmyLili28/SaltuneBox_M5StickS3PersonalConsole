#pragma once

#include <Arduino.h>

static constexpr int SCREEN_W = 135;
static constexpr int SCREEN_H = 240;

static constexpr uint16_t COLOR_BG = 0x0000;
static constexpr uint16_t COLOR_FG = 0xFFFF;
static constexpr uint16_t COLOR_MUTED = 0x8410;
static constexpr uint16_t COLOR_PINK = 0xFD78;  // PMS 1767 C approx #FCAFC0
static constexpr uint16_t COLOR_BLUE = 0x7E5D;  // PMS 297 CP approx #7ECBE8
static constexpr uint16_t COLOR_YELLOW = COLOR_PINK;
static constexpr uint16_t COLOR_GREEN = 0x07E0;
static constexpr uint16_t COLOR_RED = 0xF800;

enum AppId : uint8_t {
  APP_CLAP = 0,
  APP_DINO,
  APP_MEMO,
  APP_COIN,
  APP_WOODFISH,
  APP_RPS,
  APP_SANDTIMER,
  APP_HAIYAN,
  APP_VOLUME,
  APP_WIFI,
  APP_WEATHER,
  APP_HISTORY,
  APP_LOVELETTER,
  APP_COUNT
};

struct AppInfo {
  AppId id;
  const char* title;
  const char* icon;
};

static const AppInfo APPS[] = {
  { APP_CLAP, "Clapping Music", "/img/clapping_music.jpg" },
  { APP_DINO, "Dino", "/img/dino.jpg" },
  { APP_MEMO, "Voice Memo", "/img/voice_memos.jpg" },
  { APP_COIN, "Coin Flip", "/img/coin_flip.jpg" },
  { APP_WOODFISH, "Woodfish", "/img/woodfish.jpg" },
  { APP_RPS, "RPS", "/img/rps.jpg" },
  { APP_SANDTIMER, "Sandtimer", "/img/sandtimer.jpg" },
  { APP_HAIYAN, "Haiyan", "/img/haiyan.jpg" },
  { APP_VOLUME, "Volume", "" },
  { APP_WIFI, "WiFi Setup", "/img/wifi_setup.jpg" },
  { APP_WEATHER, "Weather", "/img/weather.jpg" },
  { APP_HISTORY, "Today History", "/img/history.jpg" },
  { APP_LOVELETTER, "", "/img/love_icon.jpg" },
};
