// brownian SCD41 by zeromem
// based on MovingIcons from LovyanGFX by https://twitter.com/lovyan03
// https://github.com/lovyan03/LovyanGFX/blob/master/examples/Sprite/MovingIcons/MovingIcons.ino
// icons replaced with air molecules (oxygen, nitrogen, carbon dioxide) for entertainment purposes only, not scientific accurate
// modified for M5Unified https://github.com/m5stack/M5Unified
// tested on M5Stack Core2 ESP32 IoT Development Kit for AWS IoT EduKit SKU: K010-AWS

// modified for ATOM Lite by riraosan_0901

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <M5GFX.h>
#include <LGFX_8BIT_CVBS.h>
#include "sfc_f65.h"
#include "molImage.h"

#define TRANSPARENT 0x0000

// 表示
LGFX_8BIT_CVBS display;

static constexpr unsigned short infoWidth   = 32;
static constexpr unsigned short infoHeight  = 32;
static constexpr unsigned short alertWidth  = 32;
static constexpr unsigned short alertHeight = 32;
static constexpr unsigned short closeWidth  = 32;
static constexpr unsigned short closeHeight = 32;

static uint32_t lcd_width;
static uint32_t lcd_height;

struct obj_info_t {
  int_fast16_t x;
  int_fast16_t y;
  int_fast16_t dx;
  int_fast16_t dy;
  int_fast8_t  img;
  float        r;
  float        z;
  float        dr;
  float        dz;

  void move() {
    r += dr;
    x += dx;
    if (x < 0) {
      x = 0;
      if (dx < 0) dx = -dx;
    } else if (x >= lcd_width) {
      x = lcd_width - 1;
      if (dx > 0) dx = -dx;
    }
    y += dy;
    if (y < 0) {
      y = 0;
      if (dy < 0) dy = -dy;
    } else if (y >= lcd_height) {
      y = lcd_height - 1;
      if (dy > 0) dy = -dy;
    }
    z += dz;
    if (z < .5) {
      z = .5;
      if (dz < .0) dz = -dz;
    } else if (z >= 2.0) {
      z = 2.0;
      if (dz > .0) dz = -dz;
    }
  }
};

static constexpr size_t obj_count = 30;
static obj_info_t       objects[obj_count];

static LGFX_Sprite  sprites[2];
static LGFX_Sprite  icons[3];
static int_fast16_t sprite_height;

static LGFX_Sprite timeSprite;

const unsigned short *f65[] = {
    sfc_43x65_f65_0,
    sfc_43x65_f65_1,
    sfc_43x65_f65_2,
    sfc_43x65_f65_3,
    sfc_43x65_f65_4,
    sfc_43x65_f65_5,
    sfc_43x65_f65_6,
    sfc_43x65_f65_7,
    sfc_43x65_f65_8,
    sfc_43x65_f65_9,
    sfc_19x65_f65_colon,
    sfc_19x65_f65_dot};

void drawTime(int h, int m, int s) {
  int hA = h / 10;
  int hR = h % 10;
  int mA = m / 10;
  int mR = m % 10;
  int sA = s / 10;
  int sR = s % 10;

  timeSprite.fillSprite(TRANSPARENT);

  int fontWidth = 43;

  // hour(24h)
  timeSprite.pushImage(fontWidth * 0, 0, fontWidth, 65, (uint16_t *)f65[hA], TRANSPARENT);
  timeSprite.pushImage(fontWidth * 1, 0, fontWidth, 65, (uint16_t *)f65[hR], TRANSPARENT);
  timeSprite.pushImage(fontWidth * 2 + 12, 0, 19, 65, (uint16_t *)f65[10], TRANSPARENT);
  // minuits
  timeSprite.pushImage(fontWidth * 3, 0, fontWidth, 65, (uint16_t *)f65[mA], TRANSPARENT);
  timeSprite.pushImage(fontWidth * 4, 0, fontWidth, 65, (uint16_t *)f65[mR], TRANSPARENT);
  timeSprite.pushImage(fontWidth * 5 + 12, 0, 19, 65, (uint16_t *)f65[10], TRANSPARENT);
  // seconds
  timeSprite.pushImage(fontWidth * 6, 0, fontWidth, 65, (uint16_t *)f65[sA], TRANSPARENT);
  timeSprite.pushImage(fontWidth * 7, 0, fontWidth, 65, (uint16_t *)f65[sR], TRANSPARENT);
}

// Connect to wifi
void setupWiFi(void) {
  WiFi.begin("俺のiPhone", "room03601");

  // Wait some time to connect to wifi
  for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
    display.setCursor(display.width() >> 1, display.height() >> 1);
    display.print("|");
    delay(500);

    display.setCursor(display.width() >> 1, display.height() >> 1);
    display.print("/");
    delay(500);

    display.setCursor(display.width() >> 1, display.height() >> 1);
    display.print("-");
    delay(500);

    display.setCursor(display.width() >> 1, display.height() >> 1);
    display.print("\\");
    delay(500);
  }

  configTzTime(PSTR("JST-9"), "ntp.nict.jp");
}

void setup(void) {
  display.init();

  if (display.width() < display.height()) {
    display.setRotation(display.getRotation() ^ 1);
  }

  lcd_width  = display.width();
  lcd_height = display.height();

  obj_info_t *a;
  for (size_t i = 0; i < obj_count; ++i) {
    a      = &objects[i];
    a->img = i % 3;
    a->x   = rand() % lcd_width;
    a->y   = rand() % lcd_height;
    a->dx  = ((rand() & 1) + 1) * (i & 1 ? 1 : -1);
    a->dy  = ((rand() & 1) + 1) * (i & 2 ? 1 : -1);
    a->dr  = ((rand() & 1) + 1) * (i & 2 ? 1 : -1);
    a->r   = 0;
    a->z   = (float)((rand() % 10) + 10) / 10;
    a->dz  = (float)((rand() % 10) + 1) / 100;
  }

  uint32_t div = 2;
  for (;;) {
    sprite_height = (lcd_height + div - 1) / div;
    bool fail     = false;
    for (std::uint32_t i = 0; !fail && i < 2; ++i) {
      sprites[i].setColorDepth(display.getColorDepth());
      sprites[i].setFont(&fonts::Font2);
      fail = !sprites[i].createSprite(lcd_width, sprite_height);
    }
    if (!fail) break;
    log_e("here");
    for (std::uint32_t i = 0; i < 2; ++i) {
      sprites[i].deleteSprite();
    }
    ++div;
  }

  icons[0].createSprite(infoWidth, infoHeight);
  icons[1].createSprite(alertWidth, alertHeight);
  icons[2].createSprite(closeWidth, closeHeight);

  icons[0].setSwapBytes(true);
  icons[1].setSwapBytes(true);
  icons[2].setSwapBytes(true);

  // replace with molecules
  icons[0].pushImage(0, 0, infoWidth, infoHeight, dioxide);
  icons[1].pushImage(0, 0, alertWidth, alertHeight, nitrogen);
  icons[2].pushImage(0, 0, closeWidth, closeHeight, oxygen);

  display.startWrite();

  setupWiFi();

  if (timeSprite.createSprite(344, 65)) {
    timeSprite.setSwapBytes(true);
  } else {
    log_e("can't allocate");
  }
}

void loop(void) {
  static uint8_t flip = 0;

  obj_info_t *a;
  for (int i = 0; i != obj_count; i++) {
    objects[i].move();
  }

  for (int_fast16_t y = 0; y < lcd_height; y += sprite_height) {
    flip = flip ? 0 : 1;
    sprites[flip].clear();
    for (size_t i = 0; i != obj_count; i++) {
      a = &objects[i];
      icons[a->img].pushRotateZoom(&sprites[flip], a->x, a->y - y, a->r, a->z, a->z, 0);
    }

    display.display();

    if (y == 0) {
      struct tm timeinfo;
      getLocalTime(&timeinfo);

      drawTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      timeSprite.pushSprite(&sprites[flip], 8, 37, TRANSPARENT);

      if ((timeinfo.tm_hour == 12) && (timeinfo.tm_min == 0) && (timeinfo.tm_sec == 0)) {
        ESP.restart();
      }
    }

    sprites[flip].pushSprite(&display, 0, y);
  }

  display.display();
}
