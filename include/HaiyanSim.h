#pragma once

#include <Arduino.h>
#include <M5Unified.h>
#include "AppConfig.h"

struct WaterParticle {
  float x;
  float y;
  float vx;
  float vy;
};

class HaiyanSim {
public:
  void begin() {
    reset();
    lastMs = millis();
    firstDraw = true;
  }

  void reset() {
    int n = 0;
    const int cols = 24;
    const int rows = 8;
    for (int y = 0; y < rows && n < PARTICLES; ++y) {
      for (int x = 0; x < cols && n < PARTICLES; ++x) {
        p[n].x = 8 + x * 5.0f + (y & 1) * 2.1f;
        p[n].y = 160 + y * 5.2f;
        p[n].vx = 0;
        p[n].vy = 0;
        oldX[n] = -1000;
        oldY[n] = -1000;
        ++n;
      }
    }
  }

  void step(float ax, float ay, float az) {
    uint32_t now = millis();
    float dt = (now - lastMs) / 1000.0f;
    lastMs = now;
    if (dt <= 0 || dt > 0.024f) dt = 0.012f;

    float mag = sqrtf(ax * ax + ay * ay + az * az);
    if (mag < 0.2f) mag = 1.0f;
    ax /= mag;
    ay /= mag;
    az /= mag;

    // IMU axes are rotated against the portrait screen; swap X/Y so water falls to the visual bottle bottom.
    tiltX = tiltX * 0.28f + (-ay) * 0.72f;
    tiltY = tiltY * 0.28f + (-ax) * 0.72f;
    float gx = tiltX * 1040.0f;
    float gy = tiltY * 1040.0f;

    for (int sub = 0; sub < 3; ++sub) {
      float sdt = dt / 3.0f;
      for (int i = 0; i < PARTICLES; ++i) {
        p[i].vx = constrain(p[i].vx + gx * sdt, -760.0f, 760.0f);
        p[i].vy = constrain(p[i].vy + gy * sdt, -760.0f, 760.0f);
        p[i].vx *= 0.989f;
        p[i].vy *= 0.989f;
        p[i].x += p[i].vx * sdt;
        p[i].y += p[i].vy * sdt;
        collide(p[i]);
      }
      separateParticles();
    }
  }

  void draw() {
    auto& d = M5.Display;
    uint16_t density[GRID_W * GRID_H];
    memset(density, 0, sizeof(density));

    for (int i = 0; i < PARTICLES; ++i) {
      int gx = constrain((int)(p[i].x * GRID_W / SCREEN_W), 0, GRID_W - 1);
      int gy = constrain((int)(p[i].y * GRID_H / SCREEN_H), 0, GRID_H - 1);
      addDensity(density, gx, gy, 150);
      addDensity(density, gx - 1, gy, 54);
      addDensity(density, gx + 1, gy, 54);
      addDensity(density, gx, gy - 1, 46);
      addDensity(density, gx, gy + 1, 46);
      addDensity(density, gx - 1, gy + 1, 24);
      addDensity(density, gx + 1, gy + 1, 24);
    }

    d.startWrite();
    drawSkyBackground();
    firstDraw = false;
    for (int gy = 0; gy < GRID_H; ++gy) {
      for (int gx = 0; gx < GRID_W; ++gx) {
        uint16_t den = density[gy * GRID_W + gx];
        if (den < 30) continue;
        int x0 = gx * SCREEN_W / GRID_W;
        int x1 = (gx + 1) * SCREEN_W / GRID_W;
        int y0 = gy * SCREEN_H / GRID_H;
        int y1 = (gy + 1) * SCREEN_H / GRID_H;
        uint8_t alpha = constrain((int)den, 84, 202);
        uint16_t c = blend565(skyColorAt((y0 + y1) / 2), waterGradient(gy), alpha);
        d.fillRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1, c);
      }
    }
    d.endWrite();
  }

  static constexpr int particleCount() { return PARTICLES; }

private:
  static constexpr int PARTICLES = 192;
  static constexpr float R = 3.05f;
  static constexpr int DRAW_R = 3;
  static constexpr int ERASE_R = 4;
  static constexpr float MIN_DIST = R * 1.58f;
  static constexpr float MIN_DIST2 = MIN_DIST * MIN_DIST;
  static constexpr float COHESION_DIST = R * 3.45f;
  static constexpr float COHESION_DIST2 = COHESION_DIST * COHESION_DIST;
  static constexpr int GRID_W = 20;
  static constexpr int GRID_H = 34;

  WaterParticle p[PARTICLES];
  float oldX[PARTICLES];
  float oldY[PARTICLES];
  uint32_t lastMs = 0;
  bool firstDraw = true;
  float tiltX = 0;
  float tiltY = 0;

  struct SkyPalette {
    uint16_t top;
    uint16_t bottom;
    uint8_t alpha;
  };

  void collide(WaterParticle& q) {
    const float left = DRAW_R;
    const float right = SCREEN_W - 1 - DRAW_R;
    const float top = DRAW_R;
    const float bottom = SCREEN_H - 1 - DRAW_R;
    if (q.x < left) { q.x = left; q.vx = fabsf(q.vx) * 0.42f; q.vy *= 0.94f; }
    if (q.x > right) { q.x = right; q.vx = -fabsf(q.vx) * 0.42f; q.vy *= 0.94f; }
    if (q.y < top) { q.y = top; q.vy = fabsf(q.vy) * 0.42f; q.vx *= 0.94f; }
    if (q.y > bottom) { q.y = bottom; q.vy = -fabsf(q.vy) * 0.42f; q.vx *= 0.96f; }
  }

  void separateParticles() {
    for (int i = 0; i < PARTICLES; ++i) {
      for (int j = i + 1; j < PARTICLES; ++j) {
        float dx = p[j].x - p[i].x;
        float dy = p[j].y - p[i].y;
        float d2 = dx * dx + dy * dy;
        if (d2 > 0.001f && d2 < MIN_DIST2) {
          float d = sqrtf(d2);
          float nx = dx / d;
          float ny = dy / d;
          float push = (MIN_DIST - d) * 0.31f;
          p[i].x -= nx * push;
          p[i].y -= ny * push;
          p[j].x += nx * push;
          p[j].y += ny * push;

          float rvx = p[j].vx - p[i].vx;
          float rvy = p[j].vy - p[i].vy;
          float vn = rvx * nx + rvy * ny;
          if (vn < 0) {
            float impulse = -vn * 0.38f;
            p[i].vx -= nx * impulse;
            p[i].vy -= ny * impulse;
            p[j].vx += nx * impulse;
            p[j].vy += ny * impulse;
          }
          collide(p[i]);
          collide(p[j]);
        } else if (d2 >= MIN_DIST2 && d2 < COHESION_DIST2) {
          float d = sqrtf(d2);
          float pull = (COHESION_DIST - d) * 0.0028f;
          float ax = dx * pull;
          float ay = dy * pull;
          p[i].vx += ax;
          p[i].vy += ay;
          p[j].vx -= ax;
          p[j].vy -= ay;
        }
      }
    }
  }

  int minuteOfDay() {
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    return t ? t->tm_hour * 60 + t->tm_min : 22 * 60;
  }

  SkyPalette skyPalette() {
    int minute = minuteOfDay();
    if (minute >= 300 && minute < 450) {
      return { M5.Display.color565(238, 112, 150), M5.Display.color565(78, 156, 232), 92 };
    }
    if ((minute >= 450 && minute < 660) || (minute >= 840 && minute < 1020)) {
      return { M5.Display.color565(88, 172, 236), M5.Display.color565(88, 220, 176), 76 };
    }
    if (minute >= 660 && minute < 840) {
      return { M5.Display.color565(78, 166, 238), M5.Display.color565(134, 205, 255), 70 };
    }
    if (minute >= 1020 && minute < 1140) {
      return { M5.Display.color565(76, 146, 228), M5.Display.color565(238, 132, 204), 92 };
    }
    if (minute >= 1140 && minute < 1260) {
      return { M5.Display.color565(8, 30, 92), M5.Display.color565(2, 8, 34), 190 };
    }
    return { M5.Display.color565(0, 0, 0), M5.Display.color565(0, 0, 0), 255 };
  }

  uint16_t skyColorAt(int y) {
    SkyPalette p = skyPalette();
    int mix = constrain(map(y, 0, SCREEN_H - 1, 0, 255), 0, 255);
    uint16_t grad = blend565(p.top, p.bottom, mix);
    return blend565(M5.Display.color565(0, 0, 0), grad, p.alpha);
  }

  void drawSkyBackground() {
    auto& d = M5.Display;
    for (int y = 0; y < SCREEN_H; y += 6) {
      d.fillRect(0, y, SCREEN_W, 6, skyColorAt(y + 3));
    }
  }

  void addDensity(uint16_t* density, int gx, int gy, uint16_t value) {
    if (gx < 0 || gx >= GRID_W || gy < 0 || gy >= GRID_H) return;
    uint16_t& cell = density[gy * GRID_W + gx];
    uint16_t next = cell + value;
    cell = next > 255 ? 255 : next;
  }

  uint16_t blend565(uint16_t bg, uint16_t fg, uint8_t alpha) {
    uint8_t br = ((bg >> 11) & 0x1F) << 3;
    uint8_t bgc = ((bg >> 5) & 0x3F) << 2;
    uint8_t bb = (bg & 0x1F) << 3;
    uint8_t fr = ((fg >> 11) & 0x1F) << 3;
    uint8_t fgx = ((fg >> 5) & 0x3F) << 2;
    uint8_t fb = (fg & 0x1F) << 3;
    uint8_t r = (br * (255 - alpha) + fr * alpha) / 255;
    uint8_t g = (bgc * (255 - alpha) + fgx * alpha) / 255;
    uint8_t b = (bb * (255 - alpha) + fb * alpha) / 255;
    return M5.Display.color565(r, g, b);
  }

  uint16_t waterGradient(int gy) {
    int mix = constrain(map(gy, 0, GRID_H - 1, 0, 255), 0, 255);
    uint8_t r = (92 * (255 - mix) + 235 * mix) / 255;
    uint8_t g = (181 * (255 - mix) + 136 * mix) / 255;
    uint8_t b = (238 * (255 - mix) + 205 * mix) / 255;
    return M5.Display.color565(r, g, b);
  }
};
