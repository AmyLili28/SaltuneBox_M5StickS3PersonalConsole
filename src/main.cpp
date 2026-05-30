#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>
#include "AppConfig.h"
#include "HaiyanSim.h"

enum ScreenMode : uint8_t { MODE_HOME, MODE_LAUNCHER, MODE_APP };

static ScreenMode mode = MODE_HOME;
static AppId selected = APP_CLAP;
static AppId running = APP_CLAP;
static uint32_t bDownAt = 0, aDownAt = 0;
static bool bWasDown = false, aWasDown = false;

static Preferences prefs;
static HaiyanSim haiyan;

static int woodfishCount = 0;
static int coinFace = -1;
static int rpsResult = -1;
static uint8_t loveStep = 0;

enum DinoState : uint8_t { DINO_INTRO, DINO_PLAY, DINO_OVER };
struct DinoObstacle {
  float x;
  uint8_t w;
  uint8_t h;
  uint8_t type;
  bool active;
};
static DinoState dinoState = DINO_INTRO;
static DinoObstacle dinoObs[3];
static float dinoY = 0;
static float dinoVy = 0;
static float dinoSpeed = 82;
static float dinoScoreF = 0;
static int dinoScore = 0;
static int dinoBest = 0;
static bool dinoGrounded = true;
static bool dinoBestLoaded = false;
static uint32_t dinoLastMs = 0;
static uint32_t dinoLastDraw = 0;
static uint32_t dinoLastSpawn = 0;
static bool dinoJumpHeld = false;
static bool dinoDuckHeld = false;
static uint32_t dinoDuckStartedAt = 0;
static uint32_t dinoJumpStart = 0;
static uint8_t dinoAirJumps = 0;
static bool dinoNight = false;
static constexpr uint8_t DINO_MAX_AIR_JUMPS = 2;
static constexpr float DINO_MAX_JUMP_Y = 76.0f;

enum ClapState : uint8_t { CLAP_MENU, CLAP_GAME, CLAP_PRACTICE, CLAP_PAUSED, CLAP_RESULT };
static ClapState clapState = CLAP_MENU;
static uint8_t clapMenu = 0;
static uint8_t clapLevel = 1;
static uint8_t clapShiftMode = 0;
static uint8_t clapStep = 0;
static uint8_t clapAdvance = 0;
static uint8_t clapLoops = 0;
static int clapHits = 0;
static int clapBeat = 0;
static int clapLastAcc = 0;
static int clapBestAcc = 0;
static int clapScore = 0;
static uint8_t clapStreak = 0;
static uint8_t clapFails = 0;
static bool clapInput[12] = {false};
static bool clapComplete = false;
static uint32_t clapLastBeat = 0;
static uint32_t clapStepStart = 0;

enum SandState : uint8_t { SAND_SELECT, SAND_RUN, SAND_PAUSE, SAND_DONE };

static uint32_t motionArmedAt = 0;
static uint32_t lastShake = 0;
static float lastAx = 0, lastAy = 0, lastAz = 1;
static bool motionPrimed = false;
static bool imuReady = false;
static int knockPhase = 0;
static uint32_t knockPhaseAt = 0;
static uint8_t systemVolume = 96;
static uint32_t volumeLastTilt = 0;
static bool volumeTiltArmed = true;
static float volumeLastX = 0;
static float volumeLastY = 0;
static bool volumePrimed = false;
static int volumeKnockPhase = 0;
static uint32_t volumeKnockPhaseAt = 0;

static SandState sandState = SAND_SELECT;
static uint8_t sandTheme = 0;
static uint16_t sandSelectSeconds = 300;
static uint16_t sandDurationSeconds = 300;
static uint32_t sandStartedAt = 0;
static uint32_t sandPausedRemainingMs = 0;
static uint32_t sandLastDraw = 0;
static uint32_t sandLastTilt = 0;
static bool sandTiltArmed = true;

static bool wifiPortalOn = false;
static WebServer server(80);
static DNSServer dnsServer;
static constexpr const char* WIFI_SETUP_SSID = "M5StickS3-Setup";
static constexpr const char* WIFI_SETUP_PASS = "12345678";
static uint32_t lastWifiApCheck = 0;
static uint32_t lastWifiStatusDraw = 0;
static uint32_t lastWifiNetCheck = 0;
static bool wifiNetCached = false;
static String wifiPortalStatus = "Ready";
static constexpr uint8_t WIFI_MAX_SAVED = 5;
static uint8_t wifiSelectedIndex = 0;
static String weatherCity = "Current";
static String weatherNowLine = "--C";
static String weatherCondition = "Refresh";
static String weatherRows[7];
static uint8_t weatherRowsCount = 0;
static int weatherIconCode = -1;
static String historyDateLabel = "--/--";
static String historyStatus = "A refresh";
static String historyEvents[8];
static int historyYears[8] = {0};
static uint8_t historyCount = 0;
static uint8_t historyIndex = 0;
static String currentPlace = "Current";
static bool timeSynced = false;
static uint32_t lastRtcSyncTry = 0;
static uint32_t lastHomeDraw = 0;

static bool memoRecording = false;
static bool memoHasFile = false;
static uint8_t memoSelected = 0;  // 0 = + New rec, 1..memoCount = saved memos
static File memoFile;
static uint32_t memoBytes = 0;
static int16_t memoBuf[512];
static constexpr uint32_t MEMO_RATE = 16000;
static constexpr uint8_t MEMO_MAX = 8;
static constexpr const char* MEMO_LEGACY_PATH = "/memo.wav";
static constexpr const char* MEMO_PATHS[MEMO_MAX] = {
  "/memo_0.wav", "/memo_1.wav", "/memo_2.wav", "/memo_3.wav",
  "/memo_4.wav", "/memo_5.wav", "/memo_6.wav", "/memo_7.wav",
};
static uint8_t memoCount = 0;
static uint8_t memoPathIndex[MEMO_MAX] = {0};
static const char* memoRecordingPath = nullptr;
static bool memoListDirty = true;

static constexpr const char* CLAP_LEVEL_NAMES[] = { "EASY", "MED", "HARD" };
static constexpr const char* CLAP_MENU_ITEMS[] = { "PLAY EASY", "PLAY MED", "PLAY HARD", "PRACTICE" };
static constexpr uint8_t CLAP_MENU_COUNT = 4;
static constexpr uint16_t CLAP_BPM[] = { 92, 104, 116 };
static constexpr uint16_t CLAP_TOLERANCE[] = { 250, 205, 165 };
static constexpr uint8_t CLAP_PASS[] = { 50, 62, 74 };
static constexpr bool CLAP_PATTERN[12] = {
  true, true, true, false, true, true, false, true, false, true, true, false
};
static constexpr uint16_t SAND_MIN_SECONDS = 0;
static constexpr uint16_t SAND_DEFAULT_SECONDS = 30;
static constexpr uint16_t SAND_MAX_SECONDS = 3600;
static constexpr uint16_t SAND_STEP_SECONDS = 30;
static constexpr uint32_t DINO_MAX_DUCK_MS = 1800;

static void refreshLocationFromNetwork();

static const char* woodfishFrames[] = {
  "/img/woodfish_frame0.jpg",
  "/img/woodfish_frame1.jpg",
  "/img/woodfish_frame2.jpg",
  "/img/woodfish_frame3.jpg",
  "/img/woodfish_frame4.jpg",
};

static void writeWavHeader(File& f, uint32_t pcmBytes) {
  struct Header {
    char riff[4]; uint32_t fileSize; char wave[4]; char fmt[4]; uint32_t fmtSize;
    uint16_t format; uint16_t channels; uint32_t sampleRate; uint32_t byteRate;
    uint16_t blockAlign; uint16_t bitsPerSample; char data[4]; uint32_t dataSize;
  } h;
  memcpy(h.riff, "RIFF", 4);
  h.fileSize = 36 + pcmBytes;
  memcpy(h.wave, "WAVE", 4);
  memcpy(h.fmt, "fmt ", 4);
  h.fmtSize = 16;
  h.format = 1;
  h.channels = 1;
  h.sampleRate = MEMO_RATE;
  h.byteRate = MEMO_RATE * 2;
  h.blockAlign = 2;
  h.bitsPerSample = 16;
  memcpy(h.data, "data", 4);
  h.dataSize = pcmBytes;
  f.seek(0);
  f.write((const uint8_t*)&h, sizeof(h));
}

static void drawCenteredText(int y, const String& text, uint16_t fg = COLOR_FG, uint16_t bg = COLOR_BG, float size = 1.0f) {
  auto& d = M5.Display;
  d.setTextDatum(middle_center);
  d.setTextColor(fg, bg);
  d.setTextSize(size);
  d.drawString(text, SCREEN_W / 2, y);
}

static bool drawImage(const char* path, int x, int y) {
  if (!path || !path[0]) return false;
  if (!LittleFS.exists(path)) return false;
  return M5.Display.drawJpgFile(LittleFS, path, x, y);
}

static void applySpeakerVolume() {
  M5.Speaker.setVolume(systemVolume);
}

static void saveSystemVolume() {
  prefs.begin("sound", false);
  prefs.putUChar("vol", systemVolume);
  prefs.end();
}

static void loadSystemVolume() {
  prefs.begin("sound", true);
  systemVolume = prefs.getUChar("vol", 96);
  prefs.end();
  systemVolume = constrain(systemVolume, (uint8_t)16, (uint8_t)160);
}

static bool readRtcNow(m5::rtc_date_t& date, m5::rtc_time_t& rtcTime) {
  return M5.Rtc.getDateTime(&date, &rtcTime) &&
         date.year >= 2024 && date.year < 2100 &&
         date.month >= 1 && date.month <= 12 &&
         date.date >= 1 && date.date <= 31 &&
         rtcTime.hours < 24 && rtcTime.minutes < 60;
}

static bool readSystemNow(m5::rtc_date_t& date, m5::rtc_time_t& rtcTime) {
  time_t now = time(nullptr);
  if (now < 1704067200) return false;
  tm t;
  if (!localtime_r(&now, &t)) return false;
  if (t.tm_year + 1900 < 2024) return false;
  date = m5::rtc_date_t(t);
  rtcTime = m5::rtc_time_t(t);
  return true;
}

static bool readClockNow(m5::rtc_date_t& date, m5::rtc_time_t& rtcTime) {
  return readRtcNow(date, rtcTime) || readSystemNow(date, rtcTime);
}

static String compactText(String text, size_t maxLen) {
  text.trim();
  if (text.length() <= maxLen) return text;
  if (maxLen < 2) return text.substring(0, maxLen);
  return text.substring(0, maxLen - 1) + "~";
}

static String cleanApiText(String text) {
  text.replace("\n", " ");
  text.replace("\r", " ");
  text.replace("  ", " ");
  text.trim();
  return text;
}

static String lineSlice(String text, size_t start, size_t maxLen) {
  if (start >= text.length()) return "";
  size_t end = min(start + maxLen, text.length());
  if (end < text.length()) {
    int space = text.lastIndexOf(' ', end);
    if (space > (int)start + 5) end = space;
  }
  String line = text.substring(start, end);
  line.trim();
  return line;
}

static void drawStatusBar() {
  auto& d = M5.Display;
  d.fillRect(0, 0, SCREEN_W, 25, COLOR_BG);
  m5::rtc_date_t date;
  m5::rtc_time_t rtcTime;
  char line[24];
  if (readClockNow(date, rtcTime)) {
    snprintf(line, sizeof(line), "%02d/%02d %02d:%02d", date.month, date.date, rtcTime.hours, rtcTime.minutes);
  } else {
    snprintf(line, sizeof(line), "--/-- --:--");
  }
  d.setTextDatum(top_left);
  d.setTextSize(1);
  d.setTextColor(COLOR_FG, COLOR_BG);
  d.drawString(line, 4, 7);

  int bat = M5.Power.getBatteryLevel();
  if (bat < 0) bat = 0;
  if (bat > 100) bat = 100;
  snprintf(line, sizeof(line), "%d%%", bat);
  uint16_t batColor = bat > 45 ? COLOR_GREEN : (bat > 20 ? COLOR_YELLOW : COLOR_RED);
  int iconX = SCREEN_W - 45;
  d.drawRoundRect(iconX, 7, 14, 8, 2, COLOR_FG);
  d.fillRect(iconX + 14, 10, 2, 3, COLOR_FG);
  d.fillRect(iconX + 2, 9, map(bat, 0, 100, 0, 10), 4, batColor);
  d.setTextDatum(top_right);
  d.drawString(line, SCREEN_W - 4, 7);
}

static void reloadMemoList() {
  memoCount = 0;
  if (LittleFS.exists(MEMO_LEGACY_PATH) && memoCount < MEMO_MAX) {
    memoPathIndex[memoCount++] = 255;
  }
  for (uint8_t i = 0; i < MEMO_MAX && memoCount < MEMO_MAX; ++i) {
    if (LittleFS.exists(MEMO_PATHS[i])) memoPathIndex[memoCount++] = i;
  }
  memoHasFile = memoCount > 0;
  if (memoSelected > memoCount) memoSelected = memoCount;
}

static const char* memoPathByListIndex(uint8_t selectedIndex) {
  if (selectedIndex == 0 || selectedIndex > memoCount) return nullptr;
  uint8_t idx = memoPathIndex[selectedIndex - 1];
  return idx == 255 ? MEMO_LEGACY_PATH : MEMO_PATHS[idx];
}

static const char* nextMemoRecordPath() {
  for (uint8_t i = 0; i < MEMO_MAX; ++i) {
    if (!LittleFS.exists(MEMO_PATHS[i])) return MEMO_PATHS[i];
  }
  LittleFS.remove(MEMO_PATHS[0]);
  return MEMO_PATHS[0];
}

static void drawAppIconPage(const AppInfo& app, int imageY = 34, int titleY = 188) {
  auto& d = M5.Display;
  d.fillScreen(COLOR_BG);
  if (!drawImage(app.icon, 0, imageY)) {
    if (app.id == APP_VOLUME) {
      int cx = SCREEN_W / 2;
      int cy = 100;
      d.fillRoundRect(cx - 34, cy - 20, 22, 40, 4, COLOR_BLUE);
      d.fillTriangle(cx - 12, cy - 28, cx - 12, cy + 28, cx + 18, cy + 12, COLOR_BLUE);
      d.drawArc(cx + 19, cy, 18, 15, 300, 60, COLOR_PINK);
      d.drawArc(cx + 19, cy, 31, 28, 300, 60, COLOR_PINK);
    } else {
      drawCenteredText(96, "Upload image", COLOR_MUTED, COLOR_BG, 1);
    }
  }
  d.fillRect(0, titleY - 16, SCREEN_W, 34, COLOR_BG);
  if (app.title && app.title[0]) drawCenteredText(titleY, app.title, COLOR_FG, COLOR_BG, 1);
}

static void drawFooter() {
  auto& d = M5.Display;
  d.fillRect(0, 214, SCREEN_W, 26, COLOR_BG);
  d.drawFastHLine(10, 214, SCREEN_W - 20, COLOR_MUTED);
  d.setTextDatum(top_left);
  d.setTextSize(1);
  d.setTextColor(COLOR_BLUE, COLOR_BG);
  d.drawString("A enter", 8, 220);
  d.setTextColor(COLOR_YELLOW, COLOR_BG);
  d.drawString("B switch", 70, 220);
}

static void drawHome() {
  auto& d = M5.Display;
  uint16_t bg = d.color565(10, 13, 18);
  uint16_t panel = d.color565(20, 22, 29);
  for (int y = 0; y < SCREEN_H; y += 6) {
    uint8_t r = map(y, 0, SCREEN_H - 1, 6, 48);
    d.fillRect(0, y, SCREEN_W, 6, d.color565(r, 5, 9));
  }
  m5::rtc_date_t date;
  m5::rtc_time_t rtcTime;
  bool hasRtc = readClockNow(date, rtcTime);
  char line[40];
  if (hasRtc) {
    static const char* week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    snprintf(line, sizeof(line), "%04d/%02d/%02d %s", date.year, date.month, date.date, week[date.weekDay % 7]);
  } else {
    snprintf(line, sizeof(line), "Time not synced");
  }
  d.setTextDatum(middle_center);
  d.setTextSize(1);
  d.setTextColor(COLOR_MUTED, bg);
  d.drawString(line, SCREEN_W / 2, 15);

  int bat = M5.Power.getBatteryLevel();
  if (bat < 0) bat = 0;
  if (bat > 100) bat = 100;
  uint16_t batColor = COLOR_PINK;

  d.drawRoundRect(22, 34, SCREEN_W - 44, 15, 4, COLOR_MUTED);
  d.fillRoundRect(25, 37, map(bat, 0, 100, 0, SCREEN_W - 50), 9, 3, batColor);
  d.setTextColor(COLOR_MUTED, bg);
  d.drawString(String("Battery ") + bat + "%", SCREEN_W / 2, 56);

  d.fillRoundRect(8, 66, SCREEN_W - 16, 76, 8, panel);
  d.setTextDatum(middle_center);
  d.setTextSize(4);
  d.setTextColor(COLOR_FG, panel);
  if (hasRtc) snprintf(line, sizeof(line), "%02d:%02d", rtcTime.hours, rtcTime.minutes);
  else snprintf(line, sizeof(line), "--:--");
  d.drawString(line, SCREEN_W / 2, 101);
  d.setTextSize(1);
  d.setTextColor(COLOR_BLUE, panel);
  d.drawString(WiFi.status() == WL_CONNECTED ? "WiFi online" : "WiFi offline", SCREEN_W / 2, 132);

  d.setTextSize(1);
  drawImage("/img/yr_logo.jpg", (SCREEN_W - 64) / 2, 149);
  d.setTextColor(d.color565(112, 18, 24), bg);
  d.drawString("Don't be afraid", SCREEN_W / 2, 190);
  d.setTextColor(COLOR_MUTED, bg);
  d.drawString("Only for Zion Yan", SCREEN_W / 2, 211);
  d.drawString("Designated by YIlin", SCREEN_W / 2, 229);
}

static void drawLauncher() {
  const AppInfo& app = APPS[selected];
  drawAppIconPage(app);
  drawStatusBar();
  drawFooter();
}

static void stopWifiPortal() {
  if (!wifiPortalOn) return;
  dnsServer.stop();
  server.stop();
  WiFi.softAPdisconnect(true);
  wifiPortalOn = false;
}

static void ensureWifiPortalAp() {
  if (!wifiPortalOn) return;
  WiFi.setSleep(false);
  wifi_mode_t m = WiFi.getMode();
  if (m == WIFI_MODE_NULL || m == WIFI_MODE_STA) {
    WiFi.mode(WIFI_AP_STA);
  }
  IPAddress apIp = WiFi.softAPIP();
  if (apIp == IPAddress(0, 0, 0, 0) || WiFi.softAPSSID() != String(WIFI_SETUP_SSID)) {
    WiFi.softAPdisconnect(false);
    WiFi.softAP(WIFI_SETUP_SSID, WIFI_SETUP_PASS, 6, false, 4);
    dnsServer.stop();
    dnsServer.start(53, "*", WiFi.softAPIP());
  }
}

static void enterLauncherAt(AppId app) {
  selected = app;
  mode = MODE_LAUNCHER;
  stopWifiPortal();
  drawLauncher();
}

static void prepareAudioForRecording();
static void prepareAudioForPlayback();

static bool playWavFile(const char* path, uint8_t volume = 255) {
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  size_t len = f.size();
  uint8_t* data = (uint8_t*)malloc(len);
  if (!data) { f.close(); return false; }
  f.read(data, len);
  f.close();
  prepareAudioForPlayback();
  M5.Speaker.setVolume(volume == 255 ? systemVolume : volume);
  bool ok = M5.Speaker.playWav(data, len, 1, -1, true);
  while (ok && M5.Speaker.isPlaying()) {
    M5.update();
    delay(1);
  }
  free(data);
  return ok;
}

static void prepareAudioForRecording() {
  M5.Speaker.stop();
  M5.Speaker.end();
  M5.Mic.end();
  delay(20);
  M5.Mic.begin();
  delay(20);
}

static void prepareAudioForPlayback() {
  M5.Speaker.stop();
  M5.Speaker.end();
  M5.Mic.end();
  delay(30);
  M5.Speaker.begin();
  applySpeakerVolume();
  delay(10);
}

static void drawVoiceMemoPlaying(uint8_t item) {
  M5.Display.fillScreen(COLOR_BG);
  drawCenteredText(24, "Voice Memo", COLOR_FG, COLOR_BG, 1);
  drawCenteredText(96, "Playing", COLOR_PINK, COLOR_BG, 2);
  drawCenteredText(136, String("Memo ") + item, COLOR_FG, COLOR_BG, 1);
}

static bool playMemoWavFile(const char* path) {
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  if (f.size() <= 44) { f.close(); return false; }
  char riff[4];
  char wave[4];
  uint32_t fileSize = 0;
  f.read((uint8_t*)riff, 4);
  f.read((uint8_t*)&fileSize, 4);
  f.read((uint8_t*)wave, 4);
  if (memcmp(riff, "RIFF", 4) || memcmp(wave, "WAVE", 4)) {
    f.close();
    return false;
  }
  f.seek(24);
  uint32_t sampleRate = 0;
  f.read((uint8_t*)&sampleRate, 4);
  if (sampleRate == 0) sampleRate = MEMO_RATE;
  f.seek(34);
  uint16_t bitsPerSample = 16;
  f.read((uint8_t*)&bitsPerSample, 2);
  if (bitsPerSample != 16) {
    f.close();
    return playWavFile(path);
  }
  f.seek(44);

  prepareAudioForPlayback();
  int16_t playBuf[2][512];
  bool played = false;
  uint8_t bufIdx = 0;
  uint32_t remaining = f.size() - 44;
  const uint8_t channel = 0;
  while (remaining > 0 && f.available()) {
    while (M5.Speaker.isPlaying(channel) >= 2) {
      M5.update();
      delay(1);
    }
    size_t want = min((uint32_t)sizeof(playBuf[bufIdx]), remaining);
    size_t bytes = f.read((uint8_t*)playBuf[bufIdx], want);
    if (bytes < 2) break;
    size_t samples = bytes / 2;
    if (!M5.Speaker.playRaw(playBuf[bufIdx], samples, sampleRate, false, 1, channel, false)) break;
    played = true;
    remaining -= bytes;
    bufIdx ^= 1;
  }
  while (M5.Speaker.isPlaying(channel)) {
    M5.update();
    delay(1);
  }
  f.close();
  return played;
}

static bool httpProbe(const char* url, uint32_t timeoutMs = 2200) {
  HTTPClient http;
  http.setTimeout(timeoutMs);
  if (!http.begin(url)) return false;
  int code = http.GET();
  http.end();
  return code == 204 || (code >= 200 && code < 400);
}

static bool fetchTextUrl(const String& url, String& payload, int& httpCode, uint32_t timeoutMs = 7000) {
  HTTPClient http;
  http.setTimeout(timeoutMs);
  bool ok = false;
  if (url.startsWith("https://")) {
    WiFiClientSecure client;
    client.setInsecure();
    ok = http.begin(client, url);
  } else {
    ok = http.begin(url);
  }
  if (!ok) {
    httpCode = -900;
    return false;
  }
  http.addHeader("User-Agent", "M5StickS3PersonalConsole/1.0");
  http.addHeader("Accept", "application/json,text/plain,*/*");
  httpCode = http.GET();
  payload = http.getString();
  http.end();
  return httpCode >= 200 && httpCode < 300;
}

static bool dnsProbe(const char* host) {
  IPAddress ip;
  return WiFi.hostByName(host, ip) == 1 && ip != IPAddress(0, 0, 0, 0);
}

static bool internetOk() {
  if (WiFi.status() != WL_CONNECTED) return false;
  static constexpr const char* probes[] = {
    "http://connect.rom.miui.com/generate_204",
    "http://connectivitycheck.platform.hicloud.com/generate_204",
    "http://captive.apple.com/hotspot-detect.html",
    "http://www.baidu.com/",
    "http://www.gstatic.com/generate_204",
    "http://connectivitycheck.gstatic.com/generate_204",
  };
  for (auto url : probes) {
    if (httpProbe(url)) return true;
  }
  return dnsProbe("ntp.aliyun.com") || dnsProbe("www.baidu.com");
}

static bool syncRtcFromNetwork(uint32_t timeoutMs = 5000) {
  if (WiFi.status() != WL_CONNECTED) return false;
  configTzTime("CST-8", "ntp.aliyun.com", "ntp.tencent.com", "cn.pool.ntp.org");
  tm t;
  if (!getLocalTime(&t, timeoutMs)) return false;
  m5::rtc_date_t rtcDate(t);
  m5::rtc_time_t rtcTime(t);
  M5.Rtc.setDateTime(&rtcDate, &rtcTime);
  timeSynced = true;
  lastRtcSyncTry = millis();
  return true;
}

static String wifiKey(const char* prefix, uint8_t index) {
  return String(prefix) + String(index);
}

static void migrateWifiPrefs() {
  prefs.begin("wifi", false);
  uint8_t count = prefs.getUChar("count", 0);
  String legacySsid = prefs.getString("ssid", "");
  if (count == 0 && legacySsid.length()) {
    prefs.putUChar("count", 1);
    prefs.putUChar("selected", 0);
    prefs.putString("s0", legacySsid);
    prefs.putString("p0", prefs.getString("pass", ""));
  }
  prefs.end();
}

static uint8_t savedWifiCount() {
  prefs.begin("wifi", true);
  uint8_t count = prefs.getUChar("count", 0);
  prefs.end();
  return count > WIFI_MAX_SAVED ? WIFI_MAX_SAVED : count;
}

static bool readSavedWifi(uint8_t index, String& ssid, String& pass) {
  ssid = "";
  pass = "";
  if (index >= savedWifiCount()) return false;
  prefs.begin("wifi", true);
  ssid = prefs.getString(wifiKey("s", index).c_str(), "");
  pass = prefs.getString(wifiKey("p", index).c_str(), "");
  prefs.end();
  return ssid.length() > 0;
}

static void loadWifiSelection() {
  uint8_t count = savedWifiCount();
  prefs.begin("wifi", true);
  wifiSelectedIndex = prefs.getUChar("selected", 0);
  prefs.end();
  if (count == 0) wifiSelectedIndex = 0;
  else if (wifiSelectedIndex >= count) wifiSelectedIndex = count - 1;
}

static void saveWifiSelection(uint8_t index) {
  uint8_t count = savedWifiCount();
  if (count == 0) {
    wifiSelectedIndex = 0;
    return;
  }
  wifiSelectedIndex = index < count ? index : count - 1;
  prefs.begin("wifi", false);
  prefs.putUChar("selected", wifiSelectedIndex);
  prefs.end();
}

static void mirrorSelectedWifi(String ssid, String pass) {
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
}

static uint8_t saveWifiNetwork(String ssid, String pass) {
  ssid.trim();
  if (!ssid.length()) return wifiSelectedIndex;
  prefs.begin("wifi", false);
  uint8_t count = prefs.getUChar("count", 0);
  if (count > WIFI_MAX_SAVED) count = WIFI_MAX_SAVED;
  uint8_t slot = count;
  for (uint8_t i = 0; i < count; ++i) {
    if (prefs.getString(wifiKey("s", i).c_str(), "") == ssid) {
      slot = i;
      break;
    }
  }
  if (slot >= WIFI_MAX_SAVED) slot = wifiSelectedIndex < WIFI_MAX_SAVED ? wifiSelectedIndex : 0;
  if (slot == count && count < WIFI_MAX_SAVED) {
    count++;
    prefs.putUChar("count", count);
  }
  prefs.putString(wifiKey("s", slot).c_str(), ssid);
  prefs.putString(wifiKey("p", slot).c_str(), pass);
  prefs.putUChar("selected", slot);
  mirrorSelectedWifi(ssid, pass);
  prefs.end();
  wifiSelectedIndex = slot;
  return slot;
}

static void deleteWifiNetwork(uint8_t index) {
  uint8_t count = savedWifiCount();
  if (index >= count) return;
  prefs.begin("wifi", false);
  for (uint8_t i = index; i + 1 < count; ++i) {
    prefs.putString(wifiKey("s", i).c_str(), prefs.getString(wifiKey("s", i + 1).c_str(), ""));
    prefs.putString(wifiKey("p", i).c_str(), prefs.getString(wifiKey("p", i + 1).c_str(), ""));
  }
  if (count > 0) {
    prefs.remove(wifiKey("s", count - 1).c_str());
    prefs.remove(wifiKey("p", count - 1).c_str());
    count--;
  }
  prefs.putUChar("count", count);
  if (count == 0) {
    prefs.remove("ssid");
    prefs.remove("pass");
    prefs.putUChar("selected", 0);
    wifiSelectedIndex = 0;
  } else {
    wifiSelectedIndex = index < count ? index : count - 1;
    prefs.putUChar("selected", wifiSelectedIndex);
    String ssid = prefs.getString(wifiKey("s", wifiSelectedIndex).c_str(), "");
    String pass = prefs.getString(wifiKey("p", wifiSelectedIndex).c_str(), "");
    mirrorSelectedWifi(ssid, pass);
  }
  prefs.end();
}

static String savedWifiSsid() {
  loadWifiSelection();
  String ssid, pass;
  readSavedWifi(wifiSelectedIndex, ssid, pass);
  return ssid;
}

static String shortText(const String& text, size_t maxLen) {
  if (text.length() <= maxLen) return text;
  if (maxLen < 2) return text.substring(0, maxLen);
  return text.substring(0, maxLen - 1) + "~";
}

static void loadCurrentPlace() {
  prefs.begin("loc", true);
  currentPlace = prefs.getString("city", "Current");
  prefs.end();
  currentPlace = compactText(currentPlace, 18);
  if (!currentPlace.length()) currentPlace = "Current";
}

static void saveCurrentPlace(String city) {
  city.trim();
  if (!city.length() || city == "Current") return;
  currentPlace = compactText(city, 18);
  prefs.begin("loc", false);
  prefs.putString("city", currentPlace);
  prefs.end();
}

static bool finishWifiConnect() {
  if (!internetOk()) return false;
  syncRtcFromNetwork(6000);
  refreshLocationFromNetwork();
  wifiPortalStatus = String("Online ") + WiFi.localIP().toString();
  wifiNetCached = true;
  lastWifiNetCheck = millis();
  return true;
}

static bool connectWifiCredential(const String& ssid, const String& pass, uint32_t timeoutMs) {
  if (!ssid.length()) return false;
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.mode(wifiPortalOn ? WIFI_AP_STA : WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    if (wifiPortalOn) {
      dnsServer.processNextRequest();
      server.handleClient();
      ensureWifiPortalAp();
    }
    delay(80);
  }
  if (finishWifiConnect()) return true;
  WiFi.disconnect(false, false);
  if (wifiPortalOn) ensureWifiPortalAp();
  return false;
}

static bool connectSelectedWifi(uint32_t timeoutMs = 6500) {
  loadWifiSelection();
  String ssid, pass;
  if (!readSavedWifi(wifiSelectedIndex, ssid, pass)) {
    wifiPortalStatus = "No saved WiFi";
    wifiNetCached = false;
    return false;
  }
  wifiPortalStatus = String("Connecting ") + shortText(ssid, 12);
  if (connectWifiCredential(ssid, pass, timeoutMs)) {
    saveWifiSelection(wifiSelectedIndex);
    wifiPortalStatus = String("Online ") + shortText(ssid, 12);
    return true;
  }
  wifiPortalStatus = String("Failed ") + shortText(ssid, 12);
  wifiNetCached = false;
  return false;
}

static bool connectSavedWifi(uint32_t timeoutMs = 6500) {
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.mode(wifiPortalOn ? WIFI_AP_STA : WIFI_STA);
  WiFi.begin();
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(80);
  }
  if (finishWifiConnect()) return true;
  WiFi.disconnect(false, false);

  loadWifiSelection();
  uint8_t count = savedWifiCount();
  for (uint8_t attempt = 0; attempt < count; ++attempt) {
    uint8_t index = (wifiSelectedIndex + attempt) % count;
    String ssid, pass;
    if (!readSavedWifi(index, ssid, pass)) continue;
    wifiPortalStatus = String("Trying ") + shortText(ssid, 12);
    if (connectWifiCredential(ssid, pass, timeoutMs)) {
      saveWifiSelection(index);
      return true;
    }
  }
  wifiNetCached = false;
  if (wifiPortalOn) ensureWifiPortalAp();
  return false;
}

static void drawCoinApp() {
  M5.Display.fillScreen(COLOR_BG);
  if (coinFace < 0) {
    drawImage(APPS[APP_COIN].icon, 0, 28);
    drawCenteredText(180, "Coin Flip", COLOR_FG, COLOR_BG, 1);
    drawCenteredText(118, "Knock / A", COLOR_MUTED, COLOR_BG, 1);
    return;
  }
  M5.Display.fillCircle(SCREEN_W / 2, 108, 42, COLOR_YELLOW);
  drawCenteredText(108, coinFace == 0 ? "HEAD" : "TAIL", COLOR_BG, COLOR_YELLOW, 1);
}

static void drawWoodfishApp(int frame = 0) {
  M5.Display.fillScreen(COLOR_BG);
  drawCenteredText(24, String(woodfishCount), COLOR_YELLOW, COLOR_BG, 2);
  drawImage(woodfishFrames[frame], 0, 58);
}

static void saveWoodfishCount() {
  prefs.begin("woodfish", false);
  prefs.putInt("count", woodfishCount);
  prefs.end();
}

static void playWoodfish() {
  woodfishCount++;
  saveWoodfishCount();
  if (!playWavFile("/audio/woodfish_knock.wav")) {
    applySpeakerVolume();
    M5.Speaker.tone(900, 45);
  }
  for (int i = 1; i < 5; ++i) {
    drawWoodfishApp(i);
    delay(45);
  }
  drawWoodfishApp(0);
}

static void drawRpsApp();

static void rollRps() {
  const char* imgs[] = { "/img/rps_rock.jpg", "/img/rps_scissors.jpg", "/img/rps_paper.jpg" };
  const int imgX = (SCREEN_W - 108) / 2;
  uint32_t start = millis();
  int i = 0;
  while (millis() - start < 760) {
    M5.Display.fillScreen(COLOR_BG);
    drawImage(imgs[i % 3], imgX, 50);
    i++;
    delay(45);
  }
  rpsResult = random(0, 3);
  drawRpsApp();
}

static void drawRpsApp() {
  M5.Display.fillScreen(COLOR_BG);
  if (rpsResult < 0) {
    drawCenteredText(88, "Knock / A", COLOR_MUTED, COLOR_BG, 1);
    return;
  }
  const char* img = rpsResult == 0 ? "/img/rps_rock.jpg" : (rpsResult == 1 ? "/img/rps_scissors.jpg" : "/img/rps_paper.jpg");
  const char* name = rpsResult == 0 ? "Rock" : (rpsResult == 1 ? "Scissors" : "Paper");
  drawImage(img, (SCREEN_W - 108) / 2, 42);
  drawCenteredText(190, name, COLOR_FG, COLOR_BG, 1);
}

static void drawLoveletterApp() {
  static constexpr const char* LOVE_IMAGES[] = {
    "/img/love_day1_closed.jpg",
    "/img/love_day1_sticks.jpg",
    "/img/love_day6575_closed.jpg",
    "/img/love_day6575_mic.jpg",
    "/img/love_day8785_closed.jpg",
    "/img/love_day8785_hearts.jpg",
    "/img/love_final.jpg",
  };
  if (loveStep >= sizeof(LOVE_IMAGES) / sizeof(LOVE_IMAGES[0])) loveStep = 0;
  M5.Display.fillScreen(COLOR_BG);
  if (!drawImage(LOVE_IMAGES[loveStep], 0, 0)) {
    drawCenteredText(92, "Mailbox", COLOR_PINK, COLOR_BG, 2);
    drawCenteredText(132, "A next", COLOR_MUTED, COLOR_BG, 1);
  }
}

static String formatDuration(uint32_t seconds) {
  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu", seconds / 60, seconds % 60);
  return String(buf);
}

static bool readAccel(float& ax, float& ay, float& az) {
  if (M5.Imu.getAccel(&ax, &ay, &az)) {
    imuReady = true;
    return true;
  }
  if (!imuReady) {
    M5.Imu.begin(&M5.In_I2C, m5::board_t::board_M5StickS3);
    if (M5.Imu.getAccel(&ax, &ay, &az)) {
      imuReady = true;
      return true;
    }
  }
  return false;
}

static uint32_t sandRemainingMs() {
  if (sandState == SAND_RUN) {
    uint32_t elapsed = millis() - sandStartedAt;
    uint32_t total = (uint32_t)sandDurationSeconds * 1000UL;
    return elapsed >= total ? 0 : total - elapsed;
  }
  if (sandState == SAND_PAUSE) return sandPausedRemainingMs;
  if (sandState == SAND_DONE) return 0;
  return (uint32_t)sandSelectSeconds * 1000UL;
}

static void drawSandtimerApp() {
  auto& d = M5.Display;
  uint16_t bg = sandTheme == 0 ? d.color565(8, 12, 20) : (sandTheme == 1 ? d.color565(22, 4, 8) : d.color565(16, 8, 24));
  uint16_t bg2 = sandTheme == 0 ? d.color565(24, 7, 14) : (sandTheme == 1 ? d.color565(58, 7, 12) : d.color565(4, 18, 42));
  uint16_t frame = sandTheme == 0 ? d.color565(194, 216, 238) : (sandTheme == 1 ? d.color565(255, 210, 218) : d.color565(178, 215, 255));
  uint16_t sandTop = COLOR_BLUE;
  uint16_t sandBottom = COLOR_PINK;
  d.fillScreen(bg);
  for (int y = 0; y < SCREEN_H; y += 8) {
    uint8_t r = sandTheme == 1 ? map(y, 0, SCREEN_H - 1, 20, 64) : map(y, 0, SCREEN_H - 1, 6, 24);
    uint8_t g = sandTheme == 2 ? map(y, 0, SCREEN_H - 1, 8, 20) : map(y, 0, SCREEN_H - 1, 8, 5);
    uint8_t b = sandTheme == 2 ? map(y, 0, SCREEN_H - 1, 24, 58) : map(y, 0, SCREEN_H - 1, 20, 10);
    d.fillRect(0, y, SCREEN_W, 8, d.color565(r, g, b));
  }

  if (sandState == SAND_SELECT) {
    int pct = map(sandSelectSeconds, SAND_MIN_SECONDS, SAND_MAX_SECONDS, 0, 100);
    d.setTextDatum(middle_center);
    d.setTextSize(1);
    d.setTextColor(frame, bg);
    d.drawString("Sandtimer", SCREEN_W / 2, 18);
    d.setTextSize(2);
    d.setTextColor(COLOR_FG, bg);
    d.drawString(formatDuration(sandSelectSeconds), SCREEN_W / 2, 54);
    d.drawRoundRect(10, 94, SCREEN_W - 20, 20, 6, frame);
    d.fillRoundRect(14, 98, map(pct, 0, 100, 0, SCREEN_W - 28), 12, 4, COLOR_BLUE);
    d.setTextDatum(top_left);
    d.setTextSize(1);
    d.setTextColor(COLOR_MUTED, bg);
    d.drawString("0", 10, 120);
    d.setTextDatum(top_right);
    d.drawString("60min", SCREEN_W - 10, 120);
    d.setTextDatum(middle_center);
    d.setTextSize(2);
    d.setTextColor(COLOR_BLUE, bg2);
    d.drawString("<", 22, 158);
    d.setTextColor(COLOR_PINK, bg2);
    d.drawString(">", SCREEN_W - 22, 158);
    d.setTextSize(1);
    d.setTextColor(COLOR_PINK, bg2);
    d.drawString("Tilt sideways", SCREEN_W / 2, 160);
    d.setTextColor(COLOR_FG, bg2);
    d.drawString("A confirm", SCREEN_W / 2, 210);
    return;
  }

  uint32_t remainMs = sandRemainingMs();
  uint32_t remain = (remainMs + 999) / 1000;
  int total = max(1, (int)sandDurationSeconds);
  int fallen = constrain(total - (int)remain, 0, total);
  int sandMax = map(sandDurationSeconds, 30, SAND_MAX_SECONDS, 18, 52);
  sandMax = constrain(sandMax, 12, 52);
  int upperH = map(total - fallen, 0, total, 0, sandMax);
  int lowerH = map(fallen, 0, total, 0, sandMax);
  int pct = map(fallen, 0, total, 0, 100);

  drawCenteredText(15, sandState == SAND_PAUSE ? "Paused" : (sandState == SAND_DONE ? "Done" : formatDuration(remain)), sandState == SAND_PAUSE ? COLOR_PINK : COLOR_FG, bg, 1);

  d.drawRoundRect(20, 58, 95, 132, 8, frame);
  d.drawFastHLine(31, 66, 73, frame);
  d.drawFastHLine(31, 182, 73, frame);
  d.drawLine(31, 66, 67, 121, frame);
  d.drawLine(104, 66, 67, 121, frame);
  d.drawLine(31, 182, 67, 129, frame);
  d.drawLine(104, 182, 67, 129, frame);
  d.fillCircle(67, 125, 3, frame);

  for (int y = 0; y < upperH; y += 4) {
    int half = map(y, 0, max(1, sandMax), 35, 3);
    int yy = 70 + y;
    d.drawFastHLine(67 - half, yy, half * 2, sandTop);
  }
  for (int y = 0; y < lowerH; y += 4) {
    int yy = 179 - y;
    int chamberHalf = map(constrain(yy, 129, 182), 129, 182, 2, 35);
    int pileHalf = map(y, 0, max(1, sandMax), 35, 5);
    int half = constrain(min(pileHalf, chamberHalf - 1), 1, 35);
    d.drawFastHLine(67 - half, yy, half * 2, sandBottom);
  }
  if (sandState == SAND_RUN && remain > 0) {
    d.drawFastVLine(67, 121, 21, sandTop);
    d.fillCircle(67, 143 + (millis() / 120) % 6, 1, sandTop);
  }
  d.drawRoundRect(16, 200, SCREEN_W - 32, 8, 3, frame);
  d.fillRoundRect(18, 202, map(pct, 0, 100, 0, SCREEN_W - 36), 4, 2, sandBottom);
  drawCenteredText(222, sandState == SAND_PAUSE ? "A resume  Hold B set" : "A pause  B theme", COLOR_MUTED, bg2, 1);
}

static void drawVolumeApp() {
  auto& d = M5.Display;
  uint16_t bg = d.color565(8, 10, 16);
  d.fillScreen(bg);
  for (int y = 0; y < SCREEN_H; y += 8) {
    d.fillRect(0, y, SCREEN_W, 8, d.color565(map(y, 0, SCREEN_H - 1, 8, 42), 5, map(y, 0, SCREEN_H - 1, 18, 34)));
  }
  int pct = map(systemVolume, 16, 160, 0, 100);
  drawCenteredText(20, "Volume", COLOR_FG, bg, 1);
  drawCenteredText(48, String(pct) + "%", COLOR_PINK, bg, 2);

  const int bars = 10;
  const int gap = 3;
  const int barW = 8;
  const int baseY = 172;
  const int startX = (SCREEN_W - (bars * barW + (bars - 1) * gap)) / 2;
  int filled = map(pct, 0, 100, 0, bars);
  for (int i = 0; i < bars; ++i) {
    int h = 14 + i * 7;
    int x = startX + i * (barW + gap);
    int y = baseY - h;
    uint16_t c = i < filled ? (i < 5 ? COLOR_BLUE : COLOR_PINK) : d.color565(40, 42, 50);
    d.fillRoundRect(x, y, barW, h, 3, c);
  }
  d.drawRoundRect(10, 188, SCREEN_W - 20, 14, 5, COLOR_MUTED);
  d.fillRoundRect(13, 191, map(pct, 0, 100, 0, SCREEN_W - 26), 8, 3, COLOR_PINK);
  drawCenteredText(214, "Down knock louder", COLOR_MUTED, bg, 1);
  drawCenteredText(228, "Up knock softer", COLOR_MUTED, bg, 1);
}

static bool clapTargetAt(uint8_t step) {
  return CLAP_PATTERN[(step + 12 - (clapShiftMode % 12)) % 12];
}

static void resetClapRun(bool practice) {
  clapState = practice ? CLAP_PRACTICE : CLAP_GAME;
  clapStep = 0;
  clapAdvance = 0;
  clapLoops = 0;
  clapHits = 0;
  clapBeat = 0;
  clapLastAcc = 0;
  clapBestAcc = 0;
  clapScore = 0;
  clapStreak = 0;
  clapFails = 0;
  clapComplete = false;
  clapLastBeat = millis();
  clapStepStart = millis();
  for (uint8_t i = 0; i < 12; ++i) clapInput[i] = false;
}

static void clearClapInput() {
  for (uint8_t i = 0; i < 12; ++i) clapInput[i] = false;
}

static int scoreClapLoop() {
  int correct = 0;
  int needed = 0;
  int extras = 0;
  for (uint8_t i = 0; i < 12; ++i) {
    bool target = clapTargetAt(i);
    if (target) needed++;
    if (clapInput[i] && target) correct++;
    else if (clapInput[i] && !target) extras++;
  }
  int acc = needed ? (correct * 100) / (needed + extras) : 0;
  if (acc > clapBestAcc) clapBestAcc = acc;
  return constrain(acc, 0, 100);
}

static void drawClapDots(int y, bool targetRow) {
  auto& d = M5.Display;
  for (uint8_t i = 0; i < 12; ++i) {
    int x = 10 + i * 10;
    bool on = targetRow ? clapTargetAt(i) : clapInput[i];
    bool now = i == clapStep;
    uint16_t off = d.color565(32, 36, 45);
    uint16_t c = on ? (targetRow ? COLOR_BLUE : COLOR_PINK) : off;
    if (now) d.drawCircle(x, y, 6, COLOR_FG);
    d.fillCircle(x, y, now ? 4 : 3, c);
    if (on && !now) d.fillCircle(x, y, 2, targetRow ? d.color565(180, 235, 255) : d.color565(255, 184, 218));
  }
}

static void drawClapPreview(int y, uint8_t shift, uint16_t color) {
  auto& d = M5.Display;
  for (uint8_t i = 0; i < 12; ++i) {
    int x = 11 + i * 10;
    bool on = CLAP_PATTERN[(i + 12 - (shift % 12)) % 12];
    d.fillCircle(x, y, on ? 3 : 2, on ? color : d.color565(32, 34, 40));
  }
}

static void drawClapApp() {
  auto& d = M5.Display;
  uint16_t bg = d.color565(12, 14, 20);
  uint16_t panel = d.color565(26, 30, 40);
  d.fillScreen(bg);
  if (clapState == CLAP_MENU) {
    drawCenteredText(18, "Clapping Music", COLOR_PINK, bg, 1);
    drawCenteredText(38, "12-step rhythm", COLOR_FG, bg, 1);
    d.setTextDatum(top_left);
    d.setTextSize(1);
    d.setTextColor(COLOR_BLUE, bg);
    d.drawString("M5", 6, 57);
    d.setTextColor(COLOR_PINK, bg);
    d.drawString("YOU", 6, 82);
    drawClapPreview(68, 0, COLOR_BLUE);
    drawClapPreview(93, clapShiftMode, COLOR_PINK);
    for (uint8_t i = 0; i < CLAP_MENU_COUNT; ++i) {
      int y = 118 + i * 23;
      bool sel = i == clapMenu;
      if (sel) d.fillRoundRect(12, y - 3, SCREEN_W - 24, 19, 5, panel);
      d.setTextDatum(middle_center);
      d.setTextColor(sel ? COLOR_PINK : COLOR_FG, sel ? panel : bg);
      d.drawString(CLAP_MENU_ITEMS[i], SCREEN_W / 2, y + 6);
    }
    drawCenteredText(220, "A start   B select", COLOR_MUTED, bg, 1);
    return;
  }
  if (clapState == CLAP_RESULT) {
    drawCenteredText(54, clapComplete ? "Complete" : "Game Over", clapComplete ? COLOR_GREEN : COLOR_RED, bg, 2);
    drawCenteredText(104, String("Score ") + clapScore, COLOR_PINK, bg, 1);
    drawCenteredText(126, String("Best ACC ") + clapBestAcc + "%", COLOR_FG, bg, 1);
    drawCenteredText(150, String("Pattern ") + (clapShiftMode + 1) + "/12", COLOR_MUTED, bg, 1);
    drawCenteredText(204, "A menu", COLOR_MUTED, bg, 1);
    return;
  }
  String title = clapState == CLAP_GAME ? "GAME" : (clapState == CLAP_PRACTICE ? "PRACTICE" : "PAUSED");
  drawCenteredText(14, title + " " + String(CLAP_LEVEL_NAMES[clapLevel]), clapState == CLAP_PAUSED ? COLOR_PINK : COLOR_FG, bg, 1);
  drawCenteredText(34, String("P") + (clapShiftMode + 1) + "/12  ADV " + clapAdvance + "/3", COLOR_MUTED, bg, 1);
  drawCenteredText(52, String("ACC ") + clapLastAcc + "%  HIT " + clapHits, COLOR_MUTED, bg, 1);
  d.setTextDatum(top_left);
  d.setTextColor(COLOR_BLUE, bg);
  d.setTextSize(1);
  d.drawString("M5", 6, 72);
  d.setTextColor(COLOR_PINK, bg);
  d.drawString("YOU", 6, 111);
  drawClapDots(84, true);
  drawClapDots(124, false);
  drawCenteredText(148, String("Pass ") + CLAP_PASS[clapLevel] + "%", COLOR_MUTED, bg, 1);
  d.drawRoundRect(10, 162, 115, 12, 4, COLOR_MUTED);
  d.fillRoundRect(12, 164, map(clapAdvance, 0, 3, 0, 111), 8, 3, COLOR_GREEN);
  drawCenteredText(190, String("Score ") + clapScore + "  Streak " + clapStreak, COLOR_FG, bg, 1);
  drawCenteredText(216, clapState == CLAP_PRACTICE ? "A clap   B shift" : "A clap   B pause", COLOR_MUTED, bg, 1);
}

static void drawVoiceMemoApp() {
  M5.Display.fillScreen(COLOR_BG);
  drawCenteredText(22, "Voice Memo", COLOR_FG, COLOR_BG, 1);
  if (memoRecording) {
    drawCenteredText(98, "Recording", COLOR_RED, COLOR_BG, 2);
    drawCenteredText(138, String(memoBytes / 32000) + "s", COLOR_FG, COLOR_BG, 1);
    drawCenteredText(200, "B save", COLOR_MUTED, COLOR_BG, 1);
    return;
  }
  if (memoListDirty) {
    reloadMemoList();
    memoListDirty = false;
  }
  drawCenteredText(72, memoSelected == 0 ? "> + New rec" : "+ New rec", memoSelected == 0 ? COLOR_YELLOW : COLOR_MUTED, COLOR_BG, 1);
  if (memoCount > 0) {
    uint8_t first = 1;
    if (memoSelected > 4) first = memoSelected - 3;
    for (uint8_t row = 0; row < 5 && first + row <= memoCount; ++row) {
      uint8_t item = first + row;
      String label = String(item == memoSelected ? "> Memo " : "Memo ") + item;
      drawCenteredText(104 + row * 22, label, item == memoSelected ? COLOR_YELLOW : COLOR_FG, COLOR_BG, 1);
    }
  } else {
    drawCenteredText(104, "No memos", COLOR_MUTED, COLOR_BG, 1);
  }
  drawCenteredText(206, "A enter  B select", COLOR_MUTED, COLOR_BG, 1);
}

static void startMemoRecord() {
  prepareAudioForRecording();
  memoRecordingPath = nextMemoRecordPath();
  memoFile = LittleFS.open(memoRecordingPath, "w");
  if (!memoFile) {
    M5.Mic.end();
    memoRecordingPath = nullptr;
    return;
  }
  uint8_t empty[44] = {0};
  memoFile.write(empty, 44);
  memoBytes = 0;
  memoRecording = true;
  memoListDirty = true;
  drawVoiceMemoApp();
}

static void stopMemoRecord() {
  if (!memoRecording) return;
  while (M5.Mic.isRecording()) {
    delay(1);
  }
  memoRecording = false;
  writeWavHeader(memoFile, memoBytes);
  memoFile.close();
  if (memoBytes == 0 && memoRecordingPath) LittleFS.remove(memoRecordingPath);
  M5.Mic.end();
  reloadMemoList();
  memoListDirty = false;
  memoSelected = memoCount;
  memoRecordingPath = nullptr;
  drawVoiceMemoApp();
}

static void updateVoiceMemo() {
  if (mode != MODE_APP || running != APP_MEMO || !memoRecording) return;
  if (M5.Mic.record(memoBuf, 512, MEMO_RATE)) {
    while (M5.Mic.isRecording()) {
      M5.update();
      delay(1);
    }
    memoFile.write((const uint8_t*)memoBuf, sizeof(memoBuf));
    memoBytes += sizeof(memoBuf);
    static uint32_t lastUi = 0;
    if (millis() - lastUi > 450) {
      lastUi = millis();
      drawVoiceMemoApp();
    }
  }
}

static String portalPage(const String& msg = "") {
  return String("<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
    "<style>body{font-family:-apple-system,BlinkMacSystemFont,sans-serif;background:#10131a;color:#fff;padding:22px}"
    "input,button{width:100%;font-size:18px;margin:8px 0;padding:12px;border-radius:10px;border:0}"
    "button{background:#60a5fa;color:#06101f}</style><h2>M5StickS3 WiFi Setup</h2>") +
    (msg.length() ? "<p>" + msg + "</p>" : "") +
    "<form method=post action=/save><input name=ssid placeholder=SSID><input name=pass placeholder=Password type=password>"
    "<button>Save and connect</button></form>";
}

static void startWifiPortal() {
  if (wifiPortalOn) return;
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(WIFI_SETUP_SSID, WIFI_SETUP_PASS, 6, false, 4);
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.on("/", [](){ server.send(200, "text/html", portalPage()); });
  server.onNotFound([](){ server.send(200, "text/html", portalPage()); });
  server.on("/save", HTTP_POST, [](){
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    ssid.trim();
    if (!ssid.length()) {
      server.send(200, "text/html", portalPage("SSID required."));
      return;
    }
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    WiFi.begin(ssid.c_str(), pass.c_str());
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
      dnsServer.processNextRequest();
      if (millis() - lastWifiApCheck > 1000) {
        lastWifiApCheck = millis();
        ensureWifiPortalAp();
      }
      delay(50);
    }
    if (internetOk()) {
      syncRtcFromNetwork(6000);
      refreshLocationFromNetwork();
      saveWifiNetwork(ssid, pass);
      wifiPortalStatus = String("Saved + online ") + WiFi.localIP().toString();
      wifiNetCached = true;
      lastWifiNetCheck = millis();
      server.send(200, "text/html", portalPage(String("Connected and saved. IP ") + WiFi.localIP().toString()));
    } else {
      bool joined = WiFi.status() == WL_CONNECTED;
      WiFi.disconnect(false, false);
      ensureWifiPortalAp();
      wifiPortalStatus = joined ? "Joined, no Internet" : "Connect failed";
      wifiNetCached = false;
      lastWifiNetCheck = millis();
      server.send(200, "text/html", portalPage(joined
        ? "WiFi joined, Internet check failed. Not saved."
        : "WiFi connection failed. Not saved."));
    }
  });
  server.begin();
  wifiPortalOn = true;
  ensureWifiPortalAp();
}

static void drawWifiApp() {
  auto& d = M5.Display;
  d.fillScreen(COLOR_BG);
  if (!wifiPortalOn) startWifiPortal();
  bool sta = WiFi.status() == WL_CONNECTED;
  if (sta && (lastWifiNetCheck == 0 || millis() - lastWifiNetCheck > 15000)) {
    wifiNetCached = internetOk();
    lastWifiNetCheck = millis();
    if (wifiNetCached) {
      syncRtcFromNetwork(4000);
      if (currentPlace == "Current") refreshLocationFromNetwork();
    }
  }
  loadWifiSelection();
  uint8_t savedCount = savedWifiCount();
  String saved = savedWifiSsid();
  String selectedPass;
  readSavedWifi(wifiSelectedIndex, saved, selectedPass);
  uint16_t stateColor = sta ? (wifiNetCached ? COLOR_GREEN : COLOR_YELLOW) : COLOR_MUTED;
  String state = sta ? (wifiNetCached ? "ONLINE" : "JOINED") : "AP SETUP";
  String savedLine = savedCount
    ? String("Saved ") + String(wifiSelectedIndex + 1) + "/" + String(savedCount) + " " + shortText(saved, 9)
    : "Saved none";

  drawCenteredText(16, "WiFi Setup", COLOR_FG, COLOR_BG, 1);
  drawCenteredText(42, state, stateColor, COLOR_BG, 1.6f);
  drawCenteredText(66, wifiPortalStatus, stateColor, COLOR_BG, 1);
  drawCenteredText(88, sta ? String("STA ") + shortText(WiFi.SSID(), 12) : "STA waiting", sta ? COLOR_BLUE : COLOR_MUTED, COLOR_BG, 1);
  drawCenteredText(106, sta ? String("IP ") + WiFi.localIP().toString() : "Phone configure", COLOR_FG, COLOR_BG, 1);
  drawCenteredText(126, savedLine, COLOR_MUTED, COLOR_BG, 1);
  drawCenteredText(150, String("AP ") + WiFi.softAPSSID(), COLOR_BLUE, COLOR_BG, 1);
  drawCenteredText(168, String("PASS ") + WIFI_SETUP_PASS, COLOR_YELLOW, COLOR_BG, 1);
  drawCenteredText(188, String("URL ") + WiFi.softAPIP().toString(), COLOR_FG, COLOR_BG, 1);
  drawCenteredText(210, wifiNetCached ? "Internet OK" : "A connect / B next", wifiNetCached ? COLOR_GREEN : COLOR_MUTED, COLOR_BG, 1);
  drawCenteredText(226, savedCount ? "Hold A forget" : "Open captive page", COLOR_MUTED, COLOR_BG, 1);
}

static void updateWifiPortal() {
  if (!wifiPortalOn) return;
  if (millis() - lastWifiApCheck > 1000) {
    lastWifiApCheck = millis();
    ensureWifiPortalAp();
  }
  dnsServer.processNextRequest();
  server.handleClient();
  if (mode == MODE_APP && running == APP_WIFI && millis() - lastWifiStatusDraw > 1000) {
    lastWifiStatusDraw = millis();
    drawWifiApp();
  }
}

static String weatherCodeText(int code) {
  if (code == 0) return "Sunny";
  if (code <= 3) return "Cloudy";
  if (code == 71 || code == 73 || code == 75 || code == 77 || code == 85 || code == 86) return "Snow";
  if (code >= 51 && code <= 67) return "Rain";
  if (code >= 80 && code <= 82) return "Rain";
  return "Cloudy";
}

static void drawWeatherIcon(int x, int y, int code) {
  auto& d = M5.Display;
  if (code == 0) {
    d.fillCircle(x, y, 12, COLOR_YELLOW);
    for (int i = 0; i < 8; ++i) {
      float a = i * PI / 4.0f;
      d.drawLine(x + cosf(a) * 17, y + sinf(a) * 17, x + cosf(a) * 22, y + sinf(a) * 22, COLOR_YELLOW);
    }
    return;
  }
  d.fillCircle(x - 10, y + 4, 10, COLOR_FG);
  d.fillCircle(x + 2, y, 14, COLOR_FG);
  d.fillCircle(x + 15, y + 6, 9, COLOR_FG);
  d.fillRect(x - 18, y + 7, 38, 10, COLOR_FG);
  if (code >= 51 && code <= 82) {
    for (int i = 0; i < 3; ++i) d.drawLine(x - 12 + i * 12, y + 24, x - 16 + i * 12, y + 34, COLOR_BLUE);
  } else if (code == 71 || code == 73 || code == 75 || code == 77 || code == 85 || code == 86) {
    for (int i = 0; i < 3; ++i) d.fillCircle(x - 12 + i * 12, y + 29, 2, COLOR_FG);
  }
}

static bool fetchLocationByIp(float& lat, float& lon, String& city) {
  String payload;
  int code = 0;
  if (fetchTextUrl("http://ip-api.com/json/?fields=status,message,city,lat,lon", payload, code, 6500)) {
    JsonDocument loc;
    if (!deserializeJson(loc, payload) && loc["status"] == "success") {
      lat = loc["lat"].as<float>();
      lon = loc["lon"].as<float>();
      city = loc["city"].as<String>();
      if (!city.length()) city = "Current";
      return true;
    }
  }

  if (fetchTextUrl("http://ipwho.is/", payload, code, 6500)) {
    JsonDocument loc;
    if (!deserializeJson(loc, payload) && loc["success"].as<bool>()) {
      lat = loc["latitude"].as<float>();
      lon = loc["longitude"].as<float>();
      city = loc["city"].as<String>();
      if (!city.length()) city = "Current";
      return true;
    }
  }
  return false;
}

static void refreshLocationFromNetwork() {
  if (WiFi.status() != WL_CONNECTED) return;
  float lat = 0;
  float lon = 0;
  String city;
  if (fetchLocationByIp(lat, lon, city)) saveCurrentPlace(city);
}

static void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED && !connectSavedWifi(9000)) {
    weatherCity = "Offline";
    weatherNowLine = "--C";
    weatherCondition = "No WiFi";
    weatherRowsCount = 0;
    return;
  }
  syncRtcFromNetwork(6000);

  float lat = 39.9042f;
  float lon = 116.4074f;
  String city = "Beijing";
  bool located = fetchLocationByIp(lat, lon, city);
  if (located) saveCurrentPlace(city);
  if (!located) {
    weatherCity = "Location failed";
    weatherNowLine = "--C";
    weatherCondition = "Check WiFi/IP";
    weatherRowsCount = 0;
    return;
  }

  String payload;
  int code = 0;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
               "&longitude=" + String(lon, 4) +
               "&current_weather=true&daily=weather_code,temperature_2m_max,temperature_2m_min&timezone=Asia%2FShanghai&forecast_days=7";
  if (!fetchTextUrl(url, payload, code, 8000)) {
    url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
          "&longitude=" + String(lon, 4) +
          "&current_weather=true&daily=weather_code,temperature_2m_max,temperature_2m_min&timezone=Asia%2FShanghai&forecast_days=7";
    if (!fetchTextUrl(url, payload, code, 8000)) {
      weatherCity = compactText(city, 18);
      weatherNowLine = "--C";
      weatherCondition = String("WX failed ") + code;
      weatherRowsCount = 0;
      return;
    }
  }
  JsonDocument wx;
  if (deserializeJson(wx, payload)) {
    weatherCity = compactText(city, 18);
    weatherNowLine = "--C";
    weatherCondition = "WX parse";
    weatherRowsCount = 0;
    return;
  }
  weatherCity = compactText(city, 18);
  float nowTemp = NAN;
  int nowCode = -1;
  if (wx["current_weather"].is<JsonObject>()) {
    nowTemp = wx["current_weather"]["temperature"].as<float>();
    nowCode = wx["current_weather"]["weathercode"].as<int>();
  }
  weatherIconCode = nowCode;
  weatherCondition = nowCode >= 0 ? weatherCodeText(nowCode) : "Today";
  weatherRowsCount = 0;
  JsonArray days = wx["daily"]["time"].as<JsonArray>();
  if (isnan(nowTemp) && days.size() > 0) {
    float hi0 = wx["daily"]["temperature_2m_max"][0].as<float>();
    float lo0 = wx["daily"]["temperature_2m_min"][0].as<float>();
    nowTemp = (hi0 + lo0) * 0.5f;
    weatherIconCode = wx["daily"]["weather_code"][0].as<int>();
    weatherCondition = weatherCodeText(weatherIconCode);
  }
  weatherNowLine = isnan(nowTemp) ? String("--C") : String((int)round(nowTemp)) + "C";
  for (int i = 0; i < 7 && i < (int)days.size(); ++i) {
    String date = days[i].as<String>();
    int hi = (int)round(wx["daily"]["temperature_2m_max"][i].as<float>());
    int lo = (int)round(wx["daily"]["temperature_2m_min"][i].as<float>());
    int wc = wx["daily"]["weather_code"][i].as<int>();
    String row = date.substring(5) + " " + String(lo) + "/" + String(hi) + " " + weatherCodeText(wc);
    weatherRows[weatherRowsCount++] = row;
  }
}

static void drawWeatherApp() {
  M5.Display.fillScreen(COLOR_BG);
  drawCenteredText(15, weatherCity, COLOR_FG, COLOR_BG, 1);
  M5.Display.setTextDatum(middle_left);
  M5.Display.setTextColor(COLOR_FG, COLOR_BG);
  M5.Display.setTextSize(2);
  M5.Display.drawString(weatherNowLine, 14, 48);
  drawWeatherIcon(102, 49, weatherIconCode);
  drawCenteredText(78, weatherCondition, COLOR_BLUE, COLOR_BG, 1);
  int y = 100;
  for (uint8_t i = 0; i < weatherRowsCount && i < 7; ++i) {
    drawCenteredText(y, weatherRows[i], COLOR_FG, COLOR_BG, 1);
    y += 16;
  }
  drawCenteredText(216, "A refresh", COLOR_MUTED, COLOR_BG, 1);
}

static bool currentMonthDay(uint8_t& month, uint8_t& day) {
  m5::rtc_date_t date;
  m5::rtc_time_t rtcTime;
  if (!readClockNow(date, rtcTime)) {
    if (WiFi.status() == WL_CONNECTED) syncRtcFromNetwork(5000);
    if (!readClockNow(date, rtcTime)) return false;
  }
  month = date.month;
  day = date.date;
  char buf[8];
  snprintf(buf, sizeof(buf), "%02d/%02d", month, day);
  historyDateLabel = buf;
  return true;
}

static void setHistoryFallback(uint8_t month, uint8_t day, const String& status) {
  historyCount = 0;
  historyIndex = 0;
  historyStatus = status;
  if (month == 5 && day == 29) {
    historyYears[0] = 1953;
    historyEvents[0] = "Edmund Hillary and Tenzing Norgay reached the summit of Mount Everest.";
    historyCount = 1;
  } else if (month == 7 && day == 20) {
    historyYears[0] = 1969;
    historyEvents[0] = "Apollo 11 landed the first humans on the Moon.";
    historyCount = 1;
  } else if (month == 10 && day == 1) {
    historyYears[0] = 1949;
    historyEvents[0] = "The People's Republic of China was founded in Beijing.";
    historyCount = 1;
  } else if (month == 12 && day == 10) {
    historyYears[0] = 1901;
    historyEvents[0] = "The first Nobel Prizes were awarded.";
    historyCount = 1;
  }
  if (historyCount == 0) {
    historyYears[0] = 0;
    historyEvents[0] = "No music history loaded. Check WiFi, then press A to retry.";
    historyCount = 1;
  }
}

static bool setMusicHistoryFallback(uint8_t month, uint8_t day, const String& status) {
  historyCount = 0;
  historyIndex = 0;
  historyStatus = status;
  if (month == 5 && day == 29) {
    historyYears[0] = 1967;
    historyEvents[0] = "Noel Gallagher, guitarist and songwriter for Oasis, was born in Manchester.";
    historyYears[1] = 1961;
    historyEvents[1] = "Melissa Etheridge, Grammy-winning rock singer-songwriter, was born.";
    historyCount = 2;
  } else if (month == 1 && day == 8) {
    historyYears[0] = 1935;
    historyEvents[0] = "Elvis Presley, one of rock and roll's defining singers, was born.";
    historyYears[1] = 1947;
    historyEvents[1] = "David Bowie, influential rock musician and songwriter, was born.";
    historyCount = 2;
  } else if (month == 2 && day == 6) {
    historyYears[0] = 1945;
    historyEvents[0] = "Bob Marley, reggae singer-songwriter and bandleader, was born.";
    historyCount = 1;
  } else if (month == 7 && day == 7) {
    historyYears[0] = 1940;
    historyEvents[0] = "Ringo Starr, drummer for the Beatles, was born.";
    historyCount = 1;
  } else if (month == 8 && day == 16) {
    historyYears[0] = 1977;
    historyEvents[0] = "Elvis Presley died, closing a defining chapter of rock and roll history.";
    historyCount = 1;
  } else if (month == 10 && day == 9) {
    historyYears[0] = 1940;
    historyEvents[0] = "John Lennon, singer-songwriter and co-founder of the Beatles, was born.";
    historyCount = 1;
  } else if (month == 11 && day == 24) {
    historyYears[0] = 1991;
    historyEvents[0] = "Freddie Mercury, lead singer of Queen, died in London.";
    historyCount = 1;
  } else if (month == 12 && day == 8) {
    historyYears[0] = 1980;
    historyEvents[0] = "John Lennon was killed in New York City.";
    historyCount = 1;
  }
  return historyCount > 0;
}

static bool isMusicHistoryText(const String& text) {
  String lower = text;
  lower.toLowerCase();
  const char* keys[] = {
    "rock", "music", "musician", "singer", "songwriter", "song", "album", "single",
    "band", "guitar", "drummer", "bassist", "vocalist", "concert", "festival",
    "record", "recording", "jazz", "blues", "punk", "metal", "pop", "beatles",
    "rolling stones", "queen", "pink floyd", "led zeppelin", "nirvana", "bowie"
  };
  for (const char* key : keys) {
    if (lower.indexOf(key) >= 0) return true;
  }
  return false;
}

static void collectHistoryItems(JsonArray items, bool musicOnly) {
  for (JsonObject item : items) {
    if (historyCount >= 8) break;
    String text = cleanApiText(item["text"].as<String>());
    if (!text.length()) continue;
    if (musicOnly && !isMusicHistoryText(text)) continue;
    historyYears[historyCount] = item["year"].as<int>();
    historyEvents[historyCount] = text;
    historyCount++;
  }
}

static bool fetchHistoryEndpoint(const String& endpoint, const char* md, bool musicOnly) {
  String payload;
  int code = 0;
  String url = String("https://api.wikimedia.org/feed/v1/wikipedia/en/onthisday/") + endpoint + "/" + md;
  if (!fetchTextUrl(url, payload, code, 9000)) {
    url = String("https://en.wikipedia.org/api/rest_v1/feed/onthisday/") + endpoint + "/" + md;
    if (!fetchTextUrl(url, payload, code, 9000)) return false;
  }
  JsonDocument doc;
  if (deserializeJson(doc, payload)) return false;
  collectHistoryItems(doc[endpoint].as<JsonArray>(), musicOnly);
  return true;
}

static void fetchHistoryToday() {
  uint8_t month = 0;
  uint8_t day = 0;
  if (!currentMonthDay(month, day)) {
    historyDateLabel = "--/--";
    setHistoryFallback(0, 0, "Need time");
    return;
  }
  if (WiFi.status() != WL_CONNECTED && !connectSavedWifi(9000)) {
    setHistoryFallback(month, day, "No WiFi");
    return;
  }

  char md[8];
  snprintf(md, sizeof(md), "%02d/%02d", month, day);
  historyCount = 0;
  historyIndex = 0;

  bool anyResponse = fetchHistoryEndpoint("events", md, true);
  if (historyCount < 8) anyResponse = fetchHistoryEndpoint("births", md, true) || anyResponse;
  if (historyCount < 8) anyResponse = fetchHistoryEndpoint("deaths", md, true) || anyResponse;
  if (historyCount == 0 && setMusicHistoryFallback(month, day, "Local Music")) {
    return;
  }
  if (historyCount == 0 && anyResponse) {
    fetchHistoryEndpoint("events", md, false);
  }
  if (historyCount == 0) {
    setHistoryFallback(month, day, anyResponse ? "No music" : "API blocked");
    return;
  }
  historyStatus = isMusicHistoryText(historyEvents[0]) ? "Music/Rock" : "Loaded";
}

static void drawHistoryApp() {
  auto& d = M5.Display;
  if (historyDateLabel == "--/--") {
    uint8_t month = 0;
    uint8_t day = 0;
    currentMonthDay(month, day);
  }
  d.fillScreen(COLOR_BG);
  drawCenteredText(15, "Today History", COLOR_YELLOW, COLOR_BG, 1);
  drawCenteredText(36, historyDateLabel, COLOR_MUTED, COLOR_BG, 1);
  bool historyOk = historyStatus == "Loaded" || historyStatus == "Music/Rock" || historyStatus == "Local Music" || historyStatus == "API blocked";
  drawCenteredText(58, historyStatus, historyOk ? COLOR_GREEN : COLOR_MUTED, COLOR_BG, 1);
  if (historyCount == 0) {
    drawCenteredText(112, "A refresh", COLOR_MUTED, COLOR_BG, 1);
    return;
  }
  uint8_t idx = historyIndex % historyCount;
  String year = historyYears[idx] ? String(historyYears[idx]) : String("Info");
  drawCenteredText(84, year, COLOR_BLUE, COLOR_BG, 2);
  String text = historyEvents[idx];
  size_t pos = 0;
  for (int row = 0; row < 5; ++row) {
    String line = lineSlice(text, pos, 20);
    if (!line.length()) break;
    drawCenteredText(116 + row * 16, line, COLOR_FG, COLOR_BG, 1);
    pos += line.length();
    while (pos < text.length() && text[pos] == ' ') pos++;
  }
  drawCenteredText(206, String(historyIndex + 1) + "/" + String(historyCount) + "  B next", COLOR_MUTED, COLOR_BG, 1);
  drawCenteredText(224, "A refresh", COLOR_MUTED, COLOR_BG, 1);
}

static void loadDinoBestOnce() {
  if (dinoBestLoaded) return;
  prefs.begin("dino", true);
  dinoBest = prefs.getInt("best", 0);
  prefs.end();
  dinoBestLoaded = true;
}

static void saveDinoBest() {
  if (dinoScore <= dinoBest) return;
  dinoBest = dinoScore;
  prefs.begin("dino", false);
  prefs.putInt("best", dinoBest);
  prefs.end();
}

static void clearDinoObstacles() {
  for (auto& o : dinoObs) o.active = false;
}

static void drawDinoSprite(int x, int groundY, bool duck) {
  auto& d = M5.Display;
  uint16_t body = dinoNight ? d.color565(238, 238, 238) : d.color565(32, 37, 45);
  uint16_t dark = dinoNight ? d.color565(18, 18, 18) : d.color565(215, 236, 255);
  if (duck) {
    d.fillRoundRect(x, groundY - 17, 31, 15, 3, body);
    d.fillRoundRect(x + 22, groundY - 24, 16, 14, 3, body);
    d.fillRect(x + 8, groundY - 2, 6, 4, body);
    d.fillRect(x + 22, groundY - 2, 6, 4, body);
    d.fillCircle(x + 33, groundY - 19, 1, dark);
  } else {
    d.fillRoundRect(x + 5, groundY - 29 - (int)dinoY, 18, 26, 3, body);
    d.fillRoundRect(x + 18, groundY - 40 - (int)dinoY, 17, 16, 3, body);
    d.fillRect(x + 11, groundY - 4 - (int)dinoY, 5, 5, body);
    d.fillRect(x + 22, groundY - 4 - (int)dinoY, 5, 5, body);
    d.fillCircle(x + 30, groundY - 35 - (int)dinoY, 1, dark);
  }
}

static void drawDinoObstacle(const DinoObstacle& o, int groundY) {
  if (!o.active) return;
  auto& d = M5.Display;
  int x = (int)o.x;
  uint16_t cactus = d.color565(74, 208, 111);
  uint16_t bird = dinoNight ? d.color565(232, 232, 232) : d.color565(48, 54, 66);
  if (o.type == 0) {
    d.fillRoundRect(x, groundY - o.h, o.w, o.h, 2, cactus);
    d.fillRect(x - 3, groundY - o.h + 8, 4, 10, cactus);
    d.fillRect(x + o.w - 1, groundY - o.h + 13, 5, 10, cactus);
  } else {
    int y = groundY - 42;
    d.fillCircle(x + 5, y, 5, bird);
    d.drawLine(x, y, x - 8, y - 5, bird);
    d.drawLine(x + 9, y, x + 17, y - 4, bird);
  }
}

static void drawDinoScene() {
  auto& d = M5.Display;
  const int groundY = 198;
  dinoNight = dinoState == DINO_PLAY && ((dinoScore / 120) % 2 == 1);
  uint16_t bg = dinoNight ? COLOR_BG : d.color565(214, 236, 248);
  uint16_t fg = dinoNight ? COLOR_FG : d.color565(24, 30, 42);
  uint16_t ground = dinoNight ? d.color565(110, 110, 110) : d.color565(70, 105, 122);
  bool duck = dinoState == DINO_PLAY && dinoDuckHeld;
  d.startWrite();
  d.fillScreen(bg);
  if (dinoNight) {
    d.fillCircle(119, 22, 6, d.color565(255, 226, 150));
    d.fillCircle(122, 19, 5, bg);
  } else {
    d.fillCircle(119, 22, 6, d.color565(255, 188, 75));
  }
  d.drawFastHLine(0, groundY, SCREEN_W, ground);
  d.setTextDatum(top_left);
  d.setTextSize(1);
  d.setTextColor(fg, bg);
  d.drawString(String("S ") + dinoScore, 4, 4);
  d.drawString(String("B ") + dinoBest, 74, 4);
  drawDinoSprite(15, groundY, duck);
  for (auto& o : dinoObs) drawDinoObstacle(o, groundY);
  if (dinoState == DINO_OVER) {
    d.fillRoundRect(16, 72, 103, 72, 5, d.color565(20, 20, 20));
    drawCenteredText(86, "Game Over", COLOR_YELLOW, d.color565(20, 20, 20), 1);
    drawCenteredText(107, String("Score ") + dinoScore, COLOR_FG, d.color565(20, 20, 20), 1);
    drawCenteredText(126, "A again / B back", COLOR_FG, d.color565(20, 20, 20), 1);
  }
  d.endWrite();
}

static void drawDinoIntro() {
  loadDinoBestOnce();
  dinoState = DINO_INTRO;
  dinoJumpHeld = false;
  dinoDuckHeld = false;
  dinoY = 0;
  dinoVy = 0;
  dinoGrounded = true;
  dinoAirJumps = 0;
  dinoNight = false;
  clearDinoObstacles();
  auto& d = M5.Display;
  d.fillScreen(COLOR_BG);
  drawCenteredText(32, "Dino", COLOR_FG, COLOR_BG, 2);
  d.drawFastHLine(12, 154, SCREEN_W - 24, d.color565(110, 110, 110));
  drawDinoSprite(20, 154, false);
  DinoObstacle demo{92, 10, 28, 0, true};
  drawDinoObstacle(demo, 154);
  drawCenteredText(178, String("Best ") + dinoBest, COLOR_YELLOW, COLOR_BG, 1);
  drawCenteredText(207, "A start", COLOR_FG, COLOR_BG, 1);
  drawCenteredText(224, "Long B back", COLOR_MUTED, COLOR_BG, 1);
}

static void startDinoGame() {
  loadDinoBestOnce();
  dinoState = DINO_PLAY;
  clearDinoObstacles();
  dinoY = 0;
  dinoVy = 0;
  dinoSpeed = 70;
  dinoScoreF = 0;
  dinoScore = 0;
  dinoGrounded = true;
  dinoJumpHeld = false;
  dinoDuckHeld = false;
  dinoAirJumps = 0;
  dinoNight = false;
  dinoJumpStart = 0;
  dinoLastMs = millis();
  dinoLastDraw = 0;
  dinoLastSpawn = millis() - 1100;
  drawDinoScene();
}

static void spawnDinoObstacle() {
  for (auto& o : dinoObs) {
    if (o.active) continue;
    o.active = true;
    o.x = SCREEN_W + random(36, 78);
    o.type = random(0, 10) < 7 ? 0 : 1;
    o.w = o.type == 0 ? random(7, 12) : 16;
    o.h = o.type == 0 ? random(18, 31) : 10;
    dinoLastSpawn = millis();
    return;
  }
}

static void startDinoJump() {
  if (dinoState != DINO_PLAY) return;
  if (dinoGrounded) {
    dinoAirJumps = 0;
    dinoGrounded = false;
    dinoVy = 320.0f;
  } else {
    if (dinoAirJumps >= DINO_MAX_AIR_JUMPS || dinoY >= DINO_MAX_JUMP_Y - 4) return;
    ++dinoAirJumps;
    dinoVy = min(360.0f, max(dinoVy, 70.0f) + 95.0f);
  }
  dinoJumpHeld = true;
  dinoJumpStart = millis();
  M5.Speaker.tone(880, 20);
}

static bool hitDinoObstacle(const DinoObstacle& o, bool duck) {
  if (!o.active) return false;
  const int groundY = 198;
  int px = duck ? 15 : 20;
  int py = duck ? groundY - 20 : groundY - 42 - (int)dinoY;
  int pw = duck ? 38 : 30;
  int ph = duck ? 18 : 40;
  int ox = (int)o.x;
  int oy = o.type == 0 ? groundY - o.h : groundY - 49;
  int ow = o.type == 0 ? o.w + 4 : 23;
  int oh = o.type == 0 ? o.h : 16;
  return px < ox + ow && px + pw > ox && py < oy + oh && py + ph > oy;
}

static void updateDino() {
  if (mode != MODE_APP || running != APP_DINO || dinoState != DINO_PLAY) return;
  uint32_t now = millis();
  if (now - dinoLastMs < 18) return;
  float dt = (now - dinoLastMs) / 1000.0f;
  if (dt > 0.045f) dt = 0.024f;
  dinoLastMs = now;
  bool bPressed = M5.BtnB.isPressed();
  if (bPressed && !dinoDuckHeld) dinoDuckStartedAt = now;
  dinoDuckHeld = bPressed && (now - dinoDuckStartedAt <= DINO_MAX_DUCK_MS);
  bool duck = dinoDuckHeld;
  if (!dinoGrounded) {
    if (dinoJumpHeld && now - dinoJumpStart < 250 && dinoVy > 60.0f && dinoY < DINO_MAX_JUMP_Y) {
      dinoVy += 340.0f * dt;
    }
    dinoVy -= 1080.0f * dt;
    dinoY += dinoVy * dt;
    if (dinoY > DINO_MAX_JUMP_Y) {
      dinoY = DINO_MAX_JUMP_Y;
      if (dinoVy > 0) dinoVy *= 0.15f;
    }
    if (dinoY <= 0) {
      dinoY = 0;
      dinoVy = 0;
      dinoGrounded = true;
      dinoAirJumps = 0;
    }
  }
  dinoSpeed = min(128.0f, 70.0f + dinoScoreF * 0.30f);
  for (auto& o : dinoObs) {
    if (!o.active) continue;
    o.x -= dinoSpeed * dt;
    if (o.x < -24) o.active = false;
    if (hitDinoObstacle(o, duck)) {
      dinoState = DINO_OVER;
      dinoJumpHeld = false;
      dinoDuckHeld = false;
      dinoAirJumps = 0;
      saveDinoBest();
      M5.Speaker.tone(180, 120);
      drawDinoScene();
      return;
    }
  }
  if (now - dinoLastSpawn > (uint32_t)max(1050, 1650 - dinoScore * 2)) spawnDinoObstacle();
  dinoScoreF += dt * 10.0f;
  dinoScore = (int)dinoScoreF;
  if (now - dinoLastDraw >= 24) {
    dinoLastDraw = now;
    drawDinoScene();
  }
}

static void updateClapMusic() {
  if (mode != MODE_APP || running != APP_CLAP || (clapState != CLAP_GAME && clapState != CLAP_PRACTICE)) return;
  uint32_t interval = 30000UL / CLAP_BPM[clapLevel];  // eighth-note grid, 12 steps per loop
  if (millis() - clapLastBeat < interval) return;
  clapLastBeat = millis();
  if (clapTargetAt(clapStep)) M5.Speaker.tone(988, 45);
  else if (clapLevel == 0) M5.Speaker.tone(520, 18);
  clapStep = (clapStep + 1) % 12;
  clapStepStart = millis();
  clapBeat++;
  if (clapStep == 0 && clapState == CLAP_GAME) {
    clapLastAcc = scoreClapLoop();
    clapLoops++;
    if (clapLastAcc >= CLAP_PASS[clapLevel]) {
      clapScore += clapLastAcc;
      clapStreak++;
      clapFails = 0;
      clapAdvance++;
      if (clapAdvance >= 3) {
        clapAdvance = 0;
        clapShiftMode++;
        if (clapShiftMode >= 12) {
          clapComplete = true;
          clapState = CLAP_RESULT;
        }
      }
    } else {
      clapStreak = 0;
      clapFails++;
      if (clapAdvance > 0) clapAdvance--;
      else if (clapFails >= 4 && clapLastAcc < CLAP_PASS[clapLevel] / 2) {
        clapComplete = false;
        clapState = CLAP_RESULT;
      }
    }
    clearClapInput();
  }
  drawClapApp();
}

static void registerClapHit() {
  if (clapState != CLAP_GAME && clapState != CLAP_PRACTICE) return;
  clapHits++;
  M5.Speaker.tone(1280 + clapLevel * 140, 28);
  uint32_t interval = 30000UL / CLAP_BPM[clapLevel];
  uint32_t elapsed = millis() - clapStepStart;
  uint8_t mark = clapStep;
  uint16_t window = CLAP_TOLERANCE[clapLevel];
  if (elapsed + window / 2 >= interval) mark = (clapStep + 1) % 12;
  clapInput[mark] = true;
  drawClapApp();
}

static void updateSandtimer() {
  if (mode != MODE_APP || running != APP_SANDTIMER) return;
  if (sandState == SAND_SELECT) {
    float ax = 0, ay = 0, az = 0;
    if (!readAccel(ax, ay, az)) return;
    float screenX = -ay;
    float screenY = -ax;
    if (fabsf(screenX) < 0.18f) sandTiltArmed = true;
    if (sandTiltArmed && millis() - sandLastTilt > 220) {
      bool horizontalTilt = fabsf(screenX) > 0.34f && fabsf(screenX) > fabsf(screenY) * 1.08f;
      if (horizontalTilt && screenX > 0.0f && sandSelectSeconds < SAND_MAX_SECONDS) {
        sandSelectSeconds = min((uint16_t)SAND_MAX_SECONDS, (uint16_t)(sandSelectSeconds + SAND_STEP_SECONDS));
        sandLastTilt = millis();
        sandTiltArmed = false;
        M5.Speaker.tone(980, 20);
        drawSandtimerApp();
      } else if (horizontalTilt && screenX < 0.0f && sandSelectSeconds > SAND_MIN_SECONDS) {
        sandSelectSeconds = sandSelectSeconds < SAND_STEP_SECONDS ? 0 : sandSelectSeconds - SAND_STEP_SECONDS;
        sandLastTilt = millis();
        sandTiltArmed = false;
        M5.Speaker.tone(520, 20);
        drawSandtimerApp();
      }
    }
    return;
  }
  if (sandState != SAND_RUN) return;
  if (sandRemainingMs() == 0) {
    sandState = SAND_DONE;
    M5.Speaker.tone(1320, 130);
    drawSandtimerApp();
    return;
  }
  if (millis() - sandLastDraw > 240) {
    sandLastDraw = millis();
    drawSandtimerApp();
  }
}

static void updateVolumeApp() {
  if (mode != MODE_APP || running != APP_VOLUME) return;
  float ax = 0, ay = 0, az = 0;
  if (!readAccel(ax, ay, az)) return;
  float screenX = -ay;
  float screenY = -ax;
  if (!volumePrimed) {
    volumeLastX = screenX;
    volumeLastY = screenY;
    volumePrimed = true;
    return;
  }
  float dx = screenX - volumeLastX;
  float dy = screenY - volumeLastY;
  volumeLastX = screenX;
  volumeLastY = screenY;
  uint32_t now = millis();
  bool verticalKnock = fabsf(dy) > 0.28f && fabsf(dy) > fabsf(dx) * 1.65f;
  if (!verticalKnock) {
    if (now - volumeKnockPhaseAt > 420) volumeKnockPhase = 0;
    return;
  }

  int dir = dy > 0 ? 1 : -1;
  if (volumeKnockPhase == 0) {
    volumeKnockPhase = dir;
    volumeKnockPhaseAt = now;
    return;
  }
  if (volumeKnockPhase != dir && now - volumeKnockPhaseAt <= 420 && now - volumeLastTilt > 520) {
    int actionDir = volumeKnockPhase;
    volumeKnockPhase = 0;
    volumeLastTilt = now;
    int next = (int)systemVolume + (actionDir > 0 ? 16 : -16);
    systemVolume = (uint8_t)constrain(next, 16, 160);
    applySpeakerVolume();
    saveSystemVolume();
    M5.Speaker.tone(actionDir > 0 ? 1120 : 620, 24);
    drawVolumeApp();
    return;
  }
  volumeKnockPhase = dir;
  volumeKnockPhaseAt = now;
}

static void resetAppState(AppId app) {
  if (memoRecording) stopMemoRecord();
  if (app != APP_WIFI) wifiPortalStatus = "Ready";
  if (app == APP_COIN) {
    coinFace = -1;
  } else if (app == APP_RPS) {
    rpsResult = -1;
  } else if (app == APP_CLAP) {
    clapState = CLAP_MENU;
    clapMenu = 0;
    clapLevel = 0;
    clapShiftMode = 0;
    clearClapInput();
    clapStep = 0;
    clapAdvance = 0;
    clapHits = 0;
    clapLastAcc = 0;
    clapScore = 0;
    clapStreak = 0;
    clapFails = 0;
  } else if (app == APP_MEMO) {
    memoSelected = 0;
    reloadMemoList();
    memoListDirty = false;
  } else if (app == APP_SANDTIMER) {
    sandState = SAND_SELECT;
    sandSelectSeconds = SAND_DEFAULT_SECONDS;
    sandDurationSeconds = SAND_DEFAULT_SECONDS;
    sandPausedRemainingMs = 0;
    sandTiltArmed = true;
    sandLastTilt = 0;
  } else if (app == APP_VOLUME) {
    volumeTiltArmed = true;
    volumePrimed = false;
    volumeLastTilt = 0;
    volumeKnockPhase = 0;
    volumeKnockPhaseAt = 0;
  } else if (app == APP_WEATHER) {
    weatherCity = "Current";
    weatherNowLine = "--C";
    weatherCondition = "A refresh";
    weatherRowsCount = 0;
    weatherIconCode = -1;
  } else if (app == APP_HISTORY) {
    historyDateLabel = "--/--";
    historyStatus = "Loading";
    historyCount = 0;
    historyIndex = 0;
  } else if (app == APP_DINO) {
    dinoState = DINO_INTRO;
    dinoDuckHeld = false;
    dinoDuckStartedAt = 0;
  } else if (app == APP_LOVELETTER) {
    loveStep = 0;
  }
}

static void drawApp() {
  switch (running) {
    case APP_COIN: drawCoinApp(); break;
    case APP_WOODFISH: drawWoodfishApp(0); break;
    case APP_RPS: drawRpsApp(); break;
    case APP_SANDTIMER: drawSandtimerApp(); break;
    case APP_HAIYAN: haiyan.begin(); break;
    case APP_CLAP: drawClapApp(); break;
    case APP_MEMO: drawVoiceMemoApp(); break;
    case APP_WIFI: drawWifiApp(); break;
    case APP_VOLUME: drawVolumeApp(); break;
    case APP_WEATHER: drawWeatherApp(); break;
    case APP_HISTORY:
      if (historyCount == 0 || historyStatus != "Loaded") {
        M5.Display.fillScreen(COLOR_BG);
        drawCenteredText(86, "Today History", COLOR_YELLOW, COLOR_BG, 1);
        drawCenteredText(116, "Loading...", COLOR_MUTED, COLOR_BG, 1);
        fetchHistoryToday();
      }
      drawHistoryApp();
      break;
    case APP_DINO: drawDinoIntro(); break;
    case APP_LOVELETTER: drawLoveletterApp(); break;
    default: break;
  }
}

static void enterApp() {
  running = selected;
  mode = MODE_APP;
  resetAppState(running);
  motionArmedAt = millis() + 900;
  motionPrimed = false;
  if (running != APP_WIFI) stopWifiPortal();
  drawApp();
}

static void handleBRelease(uint32_t heldMs) {
  if (mode == MODE_HOME) { selected = APP_CLAP; mode = MODE_LAUNCHER; drawLauncher(); return; }
  if (mode == MODE_LAUNCHER) {
    if (heldMs > 650) {
      stopWifiPortal();
      mode = MODE_HOME;
      drawHome();
    } else {
      selected = (AppId)((selected + 1) % APP_COUNT);
      drawLauncher();
    }
    return;
  }
  if (mode == MODE_APP && running == APP_DINO) {
    if (heldMs > 650) {
      if (dinoState == DINO_INTRO || dinoState == DINO_OVER) {
        enterLauncherAt(running);
      }
    } else if (dinoState == DINO_OVER) {
      drawDinoIntro();
    }
    return;
  }
  if (mode == MODE_APP && running == APP_SANDTIMER && heldMs > 650 && sandState == SAND_PAUSE) {
    sandState = SAND_SELECT;
    sandTiltArmed = true;
    drawSandtimerApp();
    return;
  }
  if (mode == MODE_APP && heldMs > 650) {
    if (memoRecording) stopMemoRecord();
    enterLauncherAt(running);
    return;
  }
  if (mode == MODE_APP) {
    if (running == APP_MEMO) {
      if (memoRecording) stopMemoRecord();
      else {
        reloadMemoList();
        memoListDirty = false;
        memoSelected = (memoSelected + 1) % (memoCount + 1);
        drawVoiceMemoApp();
      }
    } else if (running == APP_WIFI) {
      uint8_t count = savedWifiCount();
      if (count > 0) {
        loadWifiSelection();
        saveWifiSelection((wifiSelectedIndex + 1) % count);
        String ssid, pass;
        readSavedWifi(wifiSelectedIndex, ssid, pass);
        wifiPortalStatus = String("Selected ") + shortText(ssid, 12);
      } else {
        wifiPortalStatus = "No saved WiFi";
      }
      drawWifiApp();
    } else if (running == APP_HISTORY) {
      if (historyCount > 0) historyIndex = (historyIndex + 1) % historyCount;
      drawHistoryApp();
    } else if (running == APP_CLAP) {
      if (clapState == CLAP_MENU) {
        clapMenu = (clapMenu + 1) % CLAP_MENU_COUNT;
        if (clapMenu <= 2) clapLevel = clapMenu;
      }
      else if (clapState == CLAP_GAME) clapState = CLAP_PAUSED;
      else if (clapState == CLAP_PAUSED) clapState = CLAP_GAME;
      else if (clapState == CLAP_PRACTICE) clapShiftMode = (clapShiftMode + 1) % 12;
      drawClapApp();
    } else if (running == APP_SANDTIMER) {
      sandTheme = (sandTheme + 1) % 3;
      drawSandtimerApp();
    }
  }
}

static void handleARelease(uint32_t heldMs) {
  if (mode == MODE_HOME) { selected = APP_CLAP; mode = MODE_LAUNCHER; drawLauncher(); return; }
  if (mode == MODE_LAUNCHER) { enterApp(); return; }
  if (mode != MODE_APP) return;

  if (running == APP_COIN) {
    coinFace = random(0, 2); drawCoinApp();
  } else if (running == APP_RPS) {
    rollRps();
  } else if (running == APP_WOODFISH) {
    if (heldMs > 650) { woodfishCount = 0; saveWoodfishCount(); drawWoodfishApp(0); }
    else playWoodfish();
  } else if (running == APP_CLAP) {
    if (clapState == CLAP_MENU) {
      if (clapMenu <= 2) {
        clapLevel = clapMenu;
        clapShiftMode = 0;
        resetClapRun(false);
      } else {
        resetClapRun(true);
      }
    } else if (clapState == CLAP_RESULT) {
      clapState = CLAP_MENU;
    } else {
      registerClapHit();
    }
    if (clapState == CLAP_MENU || clapState == CLAP_RESULT) drawClapApp();
  } else if (running == APP_MEMO) {
    const char* memoPath = memoPathByListIndex(memoSelected);
    if (heldMs > 650 && memoPath && LittleFS.exists(memoPath)) {
      LittleFS.remove(memoPath);
      reloadMemoList();
      memoListDirty = false;
      memoSelected = 0;
      drawVoiceMemoApp();
    } else if (memoPath && LittleFS.exists(memoPath)) {
      drawVoiceMemoPlaying(memoSelected);
      playMemoWavFile(memoPath);
      drawVoiceMemoApp();
    } else if (!memoRecording) {
      startMemoRecord();
    }
  } else if (running == APP_SANDTIMER) {
    if (heldMs > 650) {
      sandState = SAND_SELECT;
      sandTiltArmed = true;
    } else if (sandState == SAND_SELECT) {
      if (sandSelectSeconds > 0) {
        sandDurationSeconds = sandSelectSeconds;
        sandState = SAND_RUN;
        sandStartedAt = millis();
        sandLastDraw = 0;
      } else {
        M5.Speaker.tone(220, 50);
      }
    } else if (sandState == SAND_RUN) {
      sandPausedRemainingMs = sandRemainingMs();
      sandState = SAND_PAUSE;
    } else if (sandState == SAND_PAUSE) {
      sandState = SAND_RUN;
      sandStartedAt = millis() - ((uint32_t)sandDurationSeconds * 1000UL - sandPausedRemainingMs);
    } else if (sandState == SAND_DONE) {
      sandState = SAND_SELECT;
      sandTiltArmed = true;
    }
    drawSandtimerApp();
  } else if (running == APP_HAIYAN) {
    haiyan.begin();
  } else if (running == APP_WIFI) {
    loadWifiSelection();
    String ssid, pass;
    bool hasNetwork = readSavedWifi(wifiSelectedIndex, ssid, pass);
    if (heldMs > 650) {
      if (hasNetwork) {
        deleteWifiNetwork(wifiSelectedIndex);
        wifiPortalStatus = String("Forgot ") + shortText(ssid, 12);
        if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == ssid) {
          WiFi.disconnect(false, false);
          wifiNetCached = false;
          ensureWifiPortalAp();
        }
      } else {
        wifiPortalStatus = "No saved WiFi";
      }
    } else {
      drawCenteredText(112, "Connecting...", COLOR_MUTED, COLOR_BG, 1);
      connectSelectedWifi(10000);
    }
    drawWifiApp();
  } else if (running == APP_VOLUME) {
    applySpeakerVolume();
    M5.Speaker.tone(880, 80);
    drawVolumeApp();
  } else if (running == APP_WEATHER) {
    drawCenteredText(112, "Loading...", COLOR_MUTED, COLOR_BG, 1);
    fetchWeather(); drawWeatherApp();
  } else if (running == APP_HISTORY) {
    drawCenteredText(112, "Loading...", COLOR_MUTED, COLOR_BG, 1);
    fetchHistoryToday(); drawHistoryApp();
  } else if (running == APP_DINO) {
    if (dinoState == DINO_INTRO || dinoState == DINO_OVER) {
      startDinoGame();
    }
  } else if (running == APP_LOVELETTER) {
    if (loveStep < 6) {
      loveStep++;
      drawLoveletterApp();
    }
  }
}

static void updateButtons() {
  bool bDown = M5.BtnB.isPressed();
  if (bDown && !bWasDown) {
    bDownAt = millis();
    if (mode == MODE_APP && running == APP_DINO && dinoState == DINO_PLAY) {
      dinoDuckHeld = true;
      dinoDuckStartedAt = millis();
    }
  }
  if (!bDown && bWasDown) {
    if (mode == MODE_APP && running == APP_DINO) {
      dinoDuckHeld = false;
    }
    handleBRelease(millis() - bDownAt);
  }
  bWasDown = bDown;

  bool aDown = M5.BtnA.isPressed();
  if (aDown && !aWasDown) {
    aDownAt = millis();
    if (mode == MODE_APP && running == APP_CLAP && (clapState == CLAP_GAME || clapState == CLAP_PRACTICE)) {
      registerClapHit();
    } else if (mode == MODE_APP && running == APP_DINO && dinoState == DINO_PLAY) {
      startDinoJump();
    }
  }
  if (!aDown && aWasDown) {
    if (mode == MODE_APP && running == APP_DINO) {
      dinoJumpHeld = false;
    }
    if (!(mode == MODE_APP && running == APP_CLAP && (clapState == CLAP_GAME || clapState == CLAP_PRACTICE))) {
      handleARelease(millis() - aDownAt);
    }
  }
  aWasDown = aDown;
}

static void updateMotionTriggers() {
  if (mode != MODE_APP) return;
  if (running != APP_COIN && running != APP_RPS && running != APP_WOODFISH) return;
  float ax = 0, ay = 0, az = 0;
  if (!readAccel(ax, ay, az)) return;
  if (millis() < motionArmedAt) {
    lastAx = ax; lastAy = ay; lastAz = az; motionPrimed = true;
    return;
  }
  if (!motionPrimed) {
    lastAx = ax; lastAy = ay; lastAz = az; motionPrimed = true;
    return;
  }
  float screenX = -ay;
  float screenY = -ax;
  float lastScreenX = -lastAy;
  float lastScreenY = -lastAx;
  float verticalStep = screenY - lastScreenY;
  float verticalMove = fabsf(verticalStep);
  float horizontalMove = fabsf(screenX - lastScreenX);
  lastAx = ax; lastAy = ay; lastAz = az;
  bool verticalKnock = verticalMove > 0.28f && verticalMove > horizontalMove * 1.65f;
  if (!verticalKnock && millis() - knockPhaseAt > 420) knockPhase = 0;
  if (verticalKnock) {
    int dir = verticalStep > 0 ? 1 : -1;
    if (knockPhase == 0) {
      knockPhase = dir;
      knockPhaseAt = millis();
      return;
    }
    if (knockPhase != dir && millis() - knockPhaseAt <= 420 && millis() - lastShake > 520) {
      knockPhase = 0;
      lastShake = millis();
      if (running == APP_COIN) { coinFace = random(0, 2); drawCoinApp(); }
      else if (running == APP_RPS) { rollRps(); }
      else if (running == APP_WOODFISH) { playWoodfish(); }
      motionArmedAt = millis() + 420;
      motionPrimed = false;
      return;
    }
    knockPhase = dir;
    knockPhaseAt = millis();
  }
}

static void updateHaiyan() {
  if (mode != MODE_APP || running != APP_HAIYAN) return;
  float ax = 0, ay = 0, az = 0;
  if (!readAccel(ax, ay, az)) return;
  haiyan.step(ax, ay, az);
  haiyan.draw();
}

static void updateHomeClock() {
  if (mode != MODE_HOME) return;
  if (millis() - lastHomeDraw < 1000) return;
  lastHomeDraw = millis();
  m5::rtc_date_t date;
  m5::rtc_time_t rtcTime;
  if (!readClockNow(date, rtcTime) && WiFi.status() == WL_CONNECTED && millis() - lastRtcSyncTry > 15000) {
    lastRtcSyncTry = millis();
    syncRtcFromNetwork(5000);
  }
  drawHome();
}

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  cfg.output_power = true;
  cfg.fallback_board = m5::board_t::board_M5StickS3;
  cfg.internal_imu = true;
  cfg.internal_spk = true;
  cfg.internal_mic = true;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(180);
  M5.Speaker.setVolume(96);
  LittleFS.begin(true);
  M5.Mic.end();
  loadSystemVolume();
  applySpeakerVolume();
  loadCurrentPlace();
  migrateWifiPrefs();
  loadWifiSelection();
  prefs.begin("woodfish", true);
  woodfishCount = prefs.getInt("count", 0);
  prefs.end();
  reloadMemoList();
  memoListDirty = false;
  randomSeed(esp_random());
  connectSavedWifi(9000);
  if (WiFi.status() == WL_CONNECTED) {
    if (!timeSynced) syncRtcFromNetwork(6000);
    if (currentPlace == "Current") refreshLocationFromNetwork();
  }
  drawHome();
  lastHomeDraw = millis();
}

void loop() {
  M5.update();
  updateButtons();
  updateMotionTriggers();
  updateVoiceMemo();
  updateWifiPortal();
  updateHaiyan();
  updateDino();
  updateClapMusic();
  updateSandtimer();
  updateVolumeApp();
  updateHomeClock();
  bool fastApp = mode == MODE_APP && (running == APP_HAIYAN || running == APP_DINO);
  if (!fastApp) delay(5);
}
