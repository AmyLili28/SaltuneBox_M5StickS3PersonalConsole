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

static uint32_t motionArmedAt = 0;
static uint32_t lastShake = 0;
static float lastAx = 0, lastAy = 0, lastAz = 1;
static bool motionPrimed = false;

static bool sandRunning = false;
static bool sandDone = false;
static uint8_t sandPreset = 0;
static uint8_t sandTheme = 0;
static uint32_t sandEndAt = 0;
static uint32_t sandLastDraw = 0;

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
static String weatherText = "A refresh";
static String weatherCity = "Current";
static String weatherNowLine = "--C";
static String weatherCondition = "Refresh";
static String weatherRows[7];
static uint8_t weatherRowsCount = 0;
static int weatherIconCode = -1;
static String currentPlace = "Current";
static bool timeSynced = false;
static uint32_t lastRtcSyncTry = 0;
static uint32_t lastWeatherFetch = 0;
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

static constexpr const char* CLAP_LEVEL_NAMES[] = { "EASY", "MED", "HARD" };
static constexpr const char* CLAP_MENU_ITEMS[] = { "PLAY EASY", "PLAY MED", "PLAY HARD", "PRACTICE" };
static constexpr uint8_t CLAP_MENU_COUNT = 4;
static constexpr uint16_t CLAP_BPM[] = { 92, 104, 116 };
static constexpr uint16_t CLAP_TOLERANCE[] = { 190, 145, 105 };
static constexpr uint8_t CLAP_PASS[] = { 55, 68, 80 };
static constexpr bool CLAP_PATTERN[12] = {
  true, true, true, false, true, true, false, true, false, true, true, false
};
static constexpr uint16_t SAND_SECONDS[] = { 30, 60, 180, 300 };

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
  if (!LittleFS.exists(path)) return false;
  String p(path);
  if (p.endsWith(".png")) return M5.Display.drawPngFile(LittleFS, path, x, y);
  return M5.Display.drawJpgFile(LittleFS, path, x, y);
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
    drawCenteredText(96, "Upload image", COLOR_MUTED, COLOR_BG, 1);
  }
  d.fillRect(0, titleY - 16, SCREEN_W, 34, COLOR_BG);
  drawCenteredText(titleY, app.title, COLOR_FG, COLOR_BG, 1);
}

static void drawFooter() {
  auto& d = M5.Display;
  d.fillRect(0, 214, SCREEN_W, 26, COLOR_BG);
  d.setTextDatum(top_left);
  d.setTextSize(1);
  d.setTextColor(COLOR_FG, COLOR_BG);
  d.drawString("A enter", 8, 220);
  d.drawString("B switch", 70, 220);
}

static void drawHome() {
  auto& d = M5.Display;
  d.fillScreen(COLOR_BG);
  m5::rtc_date_t date;
  m5::rtc_time_t rtcTime;
  bool hasRtc = readClockNow(date, rtcTime);
  char line[32];
  if (hasRtc) {
    static const char* week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    snprintf(line, sizeof(line), "%02d/%02d %s", date.month, date.date, week[date.weekDay % 7]);
  } else {
    snprintf(line, sizeof(line), "--/--");
  }
  d.setTextDatum(top_left);
  d.setTextSize(1);
  d.setTextColor(COLOR_FG, COLOR_BG);
  d.drawString(line, 8, 10);

  int bat = M5.Power.getBatteryLevel();
  if (bat < 0) bat = 0;
  if (bat > 100) bat = 100;
  snprintf(line, sizeof(line), "%d%%", bat);
  d.setTextDatum(top_right);
  d.drawString(line, SCREEN_W - 8, 10);

  drawCenteredText(34, compactText(currentPlace, 16), COLOR_MUTED, COLOR_BG, 1);

  d.setTextDatum(middle_center);
  d.setTextSize(2);
  if (hasRtc) snprintf(line, sizeof(line), "%02d:%02d", rtcTime.hours, rtcTime.minutes);
  else snprintf(line, sizeof(line), "--:--");
  d.drawString(line, SCREEN_W / 2, 64);

  d.fillRoundRect(10, 92, SCREEN_W - 20, 34, 6, COLOR_RED);
  d.fillRoundRect(14, 97, map(bat, 0, 100, 0, SCREEN_W - 28), 24, 5, COLOR_GREEN);
  d.setTextSize(1);
  d.setTextColor(COLOR_FG);
  d.drawString(String("Battery Power ") + bat + "%", SCREEN_W / 2, 109);

  drawCenteredText(145, "A menu    B menu", COLOR_BLUE, COLOR_BG, 1);
  drawCenteredText(188, "Only for Zion Yan", COLOR_FG, COLOR_BG, 1);
  drawCenteredText(210, "Designed by Yilin", COLOR_MUTED, COLOR_BG, 1);
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

static bool playWavFile(const char* path, uint8_t volume = 96) {
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  size_t len = f.size();
  uint8_t* data = (uint8_t*)malloc(len);
  if (!data) { f.close(); return false; }
  f.read(data, len);
  f.close();
  M5.Speaker.setVolume(volume);
  bool ok = M5.Speaker.playWav(data, len, 1, -1, true);
  while (ok && M5.Speaker.isPlaying()) {
    M5.update();
    delay(1);
  }
  free(data);
  return ok;
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

static String savedWifiSsid() {
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  prefs.end();
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

static bool connectSavedWifi(uint32_t timeoutMs = 6500) {
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(80);
  }
  if (internetOk()) {
    syncRtcFromNetwork(6000);
    refreshLocationFromNetwork();
    return true;
  }
  WiFi.disconnect(false, false);

  if (!ssid.length()) return false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(80);
  }
  if (internetOk()) {
    syncRtcFromNetwork(6000);
    refreshLocationFromNetwork();
    return true;
  }
  WiFi.disconnect(false, false);
  return false;
}

static void drawCoinApp() {
  M5.Display.fillScreen(COLOR_BG);
  if (coinFace < 0) {
    drawImage(APPS[APP_COIN].icon, 0, 28);
    drawCenteredText(180, "Coin Flip", COLOR_FG, COLOR_BG, 1);
    drawCenteredText(118, "Shake / A", COLOR_MUTED, COLOR_BG, 1);
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
  if (!playWavFile("/audio/woodfish_knock.wav", 132)) {
    M5.Speaker.setVolume(132);
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
    drawCenteredText(88, "Shake / A", COLOR_MUTED, COLOR_BG, 1);
    return;
  }
  const char* img = rpsResult == 0 ? "/img/rps_rock.jpg" : (rpsResult == 1 ? "/img/rps_scissors.jpg" : "/img/rps_paper.jpg");
  const char* name = rpsResult == 0 ? "Rock" : (rpsResult == 1 ? "Scissors" : "Paper");
  drawImage(img, (SCREEN_W - 108) / 2, 42);
  drawCenteredText(190, name, COLOR_FG, COLOR_BG, 1);
}

static void drawSandtimerApp() {
  auto& d = M5.Display;
  uint16_t bg = sandTheme == 0 ? d.color565(31, 43, 56) : (sandTheme == 1 ? d.color565(40, 58, 48) : d.color565(54, 43, 55));
  d.fillScreen(bg);
  drawCenteredText(28, "Sandtimer", COLOR_FG, bg, 1);
  d.drawTriangle(20, 58, 115, 58, 67, 120, COLOR_FG);
  d.drawTriangle(20, 190, 115, 190, 67, 128, COLOR_FG);
  uint32_t remain = sandRunning && sandEndAt > millis() ? (sandEndAt - millis() + 999) / 1000 : 0;
  int total = SAND_SECONDS[sandPreset];
  int fallen = sandRunning ? constrain(total - (int)remain, 0, total) : (sandDone ? total : 0);
  int upper = map(total - fallen, 0, total, 0, 28);
  int lower = map(fallen, 0, total, 0, 28);
  for (int i = 0; i < upper; ++i) d.fillCircle(36 + (i * 19) % 62, 74 + (i * 11) % 36, 2, COLOR_YELLOW);
  for (int i = 0; i < lower; ++i) d.fillCircle(34 + (i * 17) % 66, 150 + (i * 13) % 34, 2, COLOR_BLUE);
  if (sandRunning) {
    d.drawFastVLine(SCREEN_W / 2, 118, 18, COLOR_YELLOW);
  }
  drawCenteredText(208, sandRunning ? String(remain) + "s" : (sandDone ? "Done" : String(SAND_SECONDS[sandPreset]) + "s"), COLOR_FG, bg, 1);
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
    uint16_t c = on ? (targetRow ? COLOR_BLUE : COLOR_YELLOW) : d.color565(34, 38, 45);
    d.fillCircle(x, y, i == clapStep ? 4 : 3, i == clapStep ? COLOR_FG : c);
    if (on && i != clapStep) d.fillCircle(x, y, 2, c);
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
  d.fillScreen(COLOR_BG);
  if (clapState == CLAP_MENU) {
    drawCenteredText(18, "Clapping Music", COLOR_YELLOW, COLOR_BG, 1);
    drawCenteredText(38, "12-step rhythm", COLOR_FG, COLOR_BG, 1);
    d.setTextDatum(top_left);
    d.setTextSize(1);
    d.setTextColor(COLOR_BLUE, COLOR_BG);
    d.drawString("M5", 6, 57);
    d.setTextColor(COLOR_YELLOW, COLOR_BG);
    d.drawString("YOU", 6, 82);
    drawClapPreview(68, 0, COLOR_BLUE);
    drawClapPreview(93, clapShiftMode, COLOR_YELLOW);
    for (uint8_t i = 0; i < CLAP_MENU_COUNT; ++i) {
      int y = 118 + i * 23;
      bool sel = i == clapMenu;
      if (sel) d.fillRoundRect(12, y - 3, SCREEN_W - 24, 19, 5, d.color565(38, 44, 54));
      d.setTextDatum(middle_center);
      d.setTextColor(sel ? COLOR_YELLOW : COLOR_FG, sel ? d.color565(38, 44, 54) : COLOR_BG);
      d.drawString(String(sel ? "> " : "  ") + CLAP_MENU_ITEMS[i], SCREEN_W / 2, y + 6);
    }
    drawCenteredText(220, "B SELECT   A START", COLOR_MUTED, COLOR_BG, 1);
    return;
  }
  if (clapState == CLAP_RESULT) {
    drawCenteredText(54, clapComplete ? "Complete" : "Game Over", clapComplete ? COLOR_GREEN : COLOR_RED, COLOR_BG, 2);
    drawCenteredText(104, String("S ") + clapScore + "  BEST " + clapBestAcc + "%", COLOR_YELLOW, COLOR_BG, 1);
    drawCenteredText(134, String("P") + (clapShiftMode + 1) + "/12", COLOR_MUTED, COLOR_BG, 1);
    drawCenteredText(204, "A MENU", COLOR_MUTED, COLOR_BG, 1);
    return;
  }
  String title = clapState == CLAP_GAME ? "GAME" : (clapState == CLAP_PRACTICE ? "PRACTICE" : "PAUSED");
  drawCenteredText(14, title + " " + String(CLAP_LEVEL_NAMES[clapLevel]), clapState == CLAP_PAUSED ? COLOR_YELLOW : COLOR_FG, COLOR_BG, 1);
  drawCenteredText(34, String("P") + (clapShiftMode + 1) + "/12  L" + clapAdvance + "  S" + clapScore, COLOR_MUTED, COLOR_BG, 1);
  drawCenteredText(52, String("ACC ") + clapLastAcc + "%  STK " + clapStreak, COLOR_MUTED, COLOR_BG, 1);
  d.setTextDatum(top_left);
  d.setTextColor(COLOR_BLUE, COLOR_BG);
  d.setTextSize(1);
  d.drawString("M5", 6, 72);
  d.setTextColor(COLOR_YELLOW, COLOR_BG);
  d.drawString("YOU", 6, 111);
  drawClapDots(84, true);
  drawClapDots(124, false);
  drawCenteredText(148, "ADVANCE", COLOR_MUTED, COLOR_BG, 1);
  d.drawRoundRect(10, 162, 115, 12, 4, COLOR_MUTED);
  d.fillRoundRect(12, 164, map(clapAdvance, 0, 3, 0, 111), 8, 3, COLOR_GREEN);
  drawCenteredText(188, String("HITS ") + clapHits, COLOR_FG, COLOR_BG, 1);
  drawCenteredText(216, clapState == CLAP_PRACTICE ? "A CLAP   B SHIFT" : "A CLAP   B PAUSE", COLOR_MUTED, COLOR_BG, 1);
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
  reloadMemoList();
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
  memoRecordingPath = nextMemoRecordPath();
  memoFile = LittleFS.open(memoRecordingPath, "w");
  if (!memoFile) return;
  uint8_t empty[44] = {0};
  memoFile.write(empty, 44);
  memoBytes = 0;
  memoRecording = true;
  drawVoiceMemoApp();
}

static void stopMemoRecord() {
  if (!memoRecording) return;
  memoRecording = false;
  writeWavHeader(memoFile, memoBytes);
  memoFile.close();
  if (memoBytes == 0 && memoRecordingPath) LittleFS.remove(memoRecordingPath);
  reloadMemoList();
  memoSelected = memoCount;
  memoRecordingPath = nullptr;
  drawVoiceMemoApp();
}

static void updateVoiceMemo() {
  if (mode != MODE_APP || running != APP_MEMO || !memoRecording) return;
  if (M5.Mic.record(memoBuf, 512, MEMO_RATE)) {
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
      prefs.begin("wifi", false);
      prefs.putString("ssid", ssid);
      prefs.putString("pass", pass);
      prefs.end();
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
  String saved = savedWifiSsid();
  uint16_t stateColor = sta ? (wifiNetCached ? COLOR_GREEN : COLOR_YELLOW) : COLOR_MUTED;
  String state = sta ? (wifiNetCached ? "ONLINE" : "JOINED") : "AP SETUP";

  drawCenteredText(16, "WiFi Setup", COLOR_FG, COLOR_BG, 1);
  drawCenteredText(42, state, stateColor, COLOR_BG, 1.6f);
  drawCenteredText(66, wifiPortalStatus, stateColor, COLOR_BG, 1);
  drawCenteredText(88, sta ? String("STA ") + shortText(WiFi.SSID(), 12) : "STA waiting", sta ? COLOR_BLUE : COLOR_MUTED, COLOR_BG, 1);
  drawCenteredText(106, sta ? String("IP ") + WiFi.localIP().toString() : "Phone configure", COLOR_FG, COLOR_BG, 1);
  drawCenteredText(126, String("Saved ") + (saved.length() ? shortText(saved, 10) : "none"), COLOR_MUTED, COLOR_BG, 1);
  drawCenteredText(150, String("AP ") + WiFi.softAPSSID(), COLOR_BLUE, COLOR_BG, 1);
  drawCenteredText(168, String("PASS ") + WIFI_SETUP_PASS, COLOR_YELLOW, COLOR_BG, 1);
  drawCenteredText(188, String("URL ") + WiFi.softAPIP().toString(), COLOR_FG, COLOR_BG, 1);
  drawCenteredText(214, wifiNetCached ? "Internet OK" : "Open captive page", wifiNetCached ? COLOR_GREEN : COLOR_MUTED, COLOR_BG, 1);
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
    weatherText = "No WiFi";
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
    weatherText = "Location failed";
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
      weatherText = String("WX failed ") + code;
      weatherCity = compactText(city, 18);
      weatherNowLine = "--C";
      weatherCondition = weatherText;
      weatherRowsCount = 0;
      return;
    }
  }
  JsonDocument wx;
  if (deserializeJson(wx, payload)) {
    weatherText = "WX parse";
    weatherCity = compactText(city, 18);
    weatherNowLine = "--C";
    weatherCondition = weatherText;
    weatherRowsCount = 0;
    return;
  }
  weatherCity = compactText(city, 18);
  weatherText = weatherCity;
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
    weatherText += "\n" + row;
  }
  lastWeatherFetch = millis();
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

static void updateSandtimer() {
  if (mode != MODE_APP || running != APP_SANDTIMER || !sandRunning) return;
  if (millis() >= sandEndAt) {
    sandRunning = false;
    sandDone = true;
    M5.Speaker.tone(1320, 130);
    drawSandtimerApp();
    return;
  }
  if (millis() - sandLastDraw > 650) {
    sandLastDraw = millis();
    drawSandtimerApp();
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
    case APP_WEATHER: drawWeatherApp(); break;
    default: break;
  }
}

static void enterApp() {
  running = selected;
  mode = MODE_APP;
  motionArmedAt = millis() + 900;
  motionPrimed = false;
  if (running != APP_WIFI) stopWifiPortal();
  drawApp();
}

static void handleBRelease(uint32_t heldMs) {
  if (mode == MODE_HOME) { mode = MODE_LAUNCHER; drawLauncher(); return; }
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
        memoSelected = (memoSelected + 1) % (memoCount + 1);
        drawVoiceMemoApp();
      }
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
  if (mode == MODE_HOME) { mode = MODE_LAUNCHER; drawLauncher(); return; }
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
      clapHits++;
      M5.Speaker.tone(1200 + clapLevel * 120, 35);
      uint32_t interval = 30000UL / CLAP_BPM[clapLevel];
      uint32_t elapsed = millis() - clapStepStart;
      uint8_t mark = clapStep;
      if (elapsed > interval && elapsed - interval < CLAP_TOLERANCE[clapLevel]) mark = (clapStep + 1) % 12;
      clapInput[mark] = true;
    }
    drawClapApp();
  } else if (running == APP_MEMO) {
    const char* memoPath = memoPathByListIndex(memoSelected);
    if (heldMs > 650 && memoPath && LittleFS.exists(memoPath)) {
      LittleFS.remove(memoPath);
      reloadMemoList();
      memoSelected = 0;
      drawVoiceMemoApp();
    } else if (memoPath && LittleFS.exists(memoPath)) {
      playWavFile(memoPath);
      drawVoiceMemoApp();
    } else if (!memoRecording) {
      startMemoRecord();
    }
  } else if (running == APP_SANDTIMER) {
    if (heldMs > 650) {
      sandRunning = false;
      sandDone = false;
    } else {
      sandPreset = (sandPreset + (sandRunning || sandDone ? 1 : 0)) % 4;
      sandRunning = true;
      sandDone = false;
      sandEndAt = millis() + SAND_SECONDS[sandPreset] * 1000UL;
    }
    drawSandtimerApp();
  } else if (running == APP_HAIYAN) {
    haiyan.begin();
  } else if (running == APP_WEATHER) {
    drawCenteredText(112, "Loading...", COLOR_MUTED, COLOR_BG, 1);
    fetchWeather(); drawWeatherApp();
  }
}

static void updateButtons() {
  bool bDown = M5.BtnB.isPressed();
  if (bDown && !bWasDown) bDownAt = millis();
  if (!bDown && bWasDown) handleBRelease(millis() - bDownAt);
  bWasDown = bDown;

  bool aDown = M5.BtnA.isPressed();
  if (aDown && !aWasDown) aDownAt = millis();
  if (!aDown && aWasDown) handleARelease(millis() - aDownAt);
  aWasDown = aDown;
}

static void updateMotionTriggers() {
  if (mode != MODE_APP) return;
  if (running != APP_COIN && running != APP_RPS && running != APP_WOODFISH) return;
  float ax = 0, ay = 0, az = 0;
  M5.Imu.getAccel(&ax, &ay, &az);
  if (millis() < motionArmedAt) {
    lastAx = ax; lastAy = ay; lastAz = az; motionPrimed = true;
    return;
  }
  if (!motionPrimed) {
    lastAx = ax; lastAy = ay; lastAz = az; motionPrimed = true;
    return;
  }
  float jerk = fabsf(ax - lastAx) + fabsf(ay - lastAy) + fabsf(az - lastAz);
  float motion = fabsf(ax) + fabsf(ay) + fabsf(az - 1.0f);
  lastAx = ax; lastAy = ay; lastAz = az;
  if (jerk > 0.85f && motion > 1.55f && millis() - lastShake > 1050) {
    lastShake = millis();
    if (running == APP_COIN) { coinFace = random(0, 2); drawCoinApp(); }
    else if (running == APP_RPS) { rollRps(); }
    else if (running == APP_WOODFISH) { playWoodfish(); }
    motionArmedAt = millis() + 500;
    motionPrimed = false;
  }
}

static void updateHaiyan() {
  if (mode != MODE_APP || running != APP_HAIYAN) return;
  float ax = 0, ay = 0, az = 0;
  M5.Imu.getAccel(&ax, &ay, &az);
  haiyan.step(ax, ay);
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
  cfg.internal_imu = true;
  cfg.internal_spk = true;
  cfg.internal_mic = true;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(180);
  M5.Speaker.setVolume(96);
  LittleFS.begin(true);
  M5.Mic.begin();
  loadCurrentPlace();
  prefs.begin("woodfish", true);
  woodfishCount = prefs.getInt("count", 0);
  prefs.end();
  reloadMemoList();
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
  updateClapMusic();
  updateSandtimer();
  updateHomeClock();
  if (!(mode == MODE_APP && running == APP_HAIYAN)) delay(5);
}
