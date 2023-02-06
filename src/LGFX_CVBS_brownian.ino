// brownian SCD41 by zeromem
// based on MovingIcons from LovyanGFX by https://twitter.com/lovyan03
// https://github.com/lovyan03/LovyanGFX/blob/master/examples/Sprite/MovingIcons/MovingIcons.ino
// icons replaced with air molecules (oxygen, nitrogen, carbon dioxide) for entertainment purposes only, not scientific accurate
// modified for M5Unified https://github.com/m5stack/M5Unified
// tested on M5Stack Core2 ESP32 IoT Development Kit for AWS IoT EduKit SKU: K010-AWS

// modified by @riraosan_0901 for M5Stack ATOM Lite

#include <vector>
#include <algorithm>
#include <functional>

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <M5GFX.h>
#include <LGFX_8BIT_CVBS.h>
#include "sfc_f65.h"
#include <Button2.h>

#define TFCARD_CS_PIN 4
#define LGFX          LGFX_8BIT_CVBS

#define LGFX_ONLY
#define USE_DISPLAY

#define SDU_APP_NAME "2D3D NTP Clock"
#define SDU_APP_PATH "/2D3D_NTP_CLOCK.bin"

#include <M5StackUpdater.h>

#define TRANSPARENT 0x0000

// 表示
static LGFX_8BIT_CVBS display(25);//Select G26 or G25
static Button2        button;

static uint32_t lcd_width;
static uint32_t lcd_height;

struct obj_info_t {
  int_fast16_t x;
  int_fast16_t y;
  int_fast16_t dx;
  int_fast16_t dy;
  int_fast8_t  img;
  int_fast16_t width;
  int_fast16_t height;
  int_fast16_t transX;
  int_fast16_t transY;
  uint32_t     color;
  float        z;
  float        r;
  float        dr;
  float        dz;
  float        _scale;

  void translate(int_fast16_t trans_x, int_fast16_t trans_y) {
    transX = trans_x;
    transY = trans_y;
  }

  void draw3D(void) {
    _scale = 1.0F / z;

    dx = transX + x * _scale;
    dy = transY + y * _scale;

    z -= 0.1;
    if (z < 0) {
      z = 100;
      color = display.color888(random(255), random(255), random(255));
    }
    // log_d("x=%d, y=%d, scale=%.4f", x, y, _scale);
  }

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
std::vector<obj_info_t> obj;

static M5Canvas     sprites[2];
static M5Canvas     rectangle;
static int_fast16_t sprite_height;
static int_fast16_t time_height;

static M5Canvas timeSprite;

static uint32_t colors[obj_count];

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

bool operator<(const obj_info_t &left, const obj_info_t &right) {
  return left.z < right.z;
}

bool operator>(const obj_info_t &left, const obj_info_t &right) {
  return left.z > right.z;
}

void drawTime(int flip, int h, int m, int s) {
  int hA = h / 10;
  int hR = h % 10;
  int mA = m / 10;
  int mR = m % 10;
  int sA = s / 10;
  int sR = s % 10;

  int fontWidth = 10;
  int offset_y  = 10;

  // hour(24h)
  fontWidth += 0;
  timeSprite.fillSprite(TRANSPARENT);
  timeSprite.pushImage(0, 0, 43, 65, (uint16_t *)f65[hA], TRANSPARENT);
  timeSprite.pushSprite(&sprites[flip], fontWidth, offset_y, TRANSPARENT);

  fontWidth += 43;
  timeSprite.fillSprite(TRANSPARENT);
  timeSprite.pushImage(0, 0, 43, 65, (uint16_t *)f65[hR], TRANSPARENT);
  timeSprite.pushSprite(&sprites[flip], fontWidth, offset_y, TRANSPARENT);

  fontWidth += 43;
  timeSprite.fillSprite(TRANSPARENT);
  timeSprite.pushImage(0, 0, 19, 65, (uint16_t *)f65[10], TRANSPARENT);
  timeSprite.pushSprite(&sprites[flip], fontWidth, offset_y, TRANSPARENT);

  // minuits
  fontWidth += 19;
  timeSprite.fillSprite(TRANSPARENT);
  timeSprite.pushImage(0, 0, 43, 65, (uint16_t *)f65[mA], TRANSPARENT);
  timeSprite.pushSprite(&sprites[flip], fontWidth, offset_y, TRANSPARENT);

  fontWidth += 43;
  timeSprite.fillSprite(TRANSPARENT);
  timeSprite.pushImage(0, 0, 43, 65, (uint16_t *)f65[mR], TRANSPARENT);
  timeSprite.pushSprite(&sprites[flip], fontWidth, offset_y, TRANSPARENT);

  fontWidth += 43;
  timeSprite.fillSprite(TRANSPARENT);
  timeSprite.pushImage(0, 0, 19, 65, (uint16_t *)f65[10], TRANSPARENT);
  timeSprite.pushSprite(&sprites[flip], fontWidth, offset_y, TRANSPARENT);

  // seconds
  fontWidth += 19;
  timeSprite.fillSprite(TRANSPARENT);
  timeSprite.pushImage(0, 0, 43, 65, (uint16_t *)f65[sA], TRANSPARENT);
  timeSprite.pushSprite(&sprites[flip], fontWidth, offset_y, TRANSPARENT);

  fontWidth += 43;
  timeSprite.fillSprite(TRANSPARENT);
  timeSprite.pushImage(0, 0, 43, 65, (uint16_t *)f65[sR], TRANSPARENT);
  timeSprite.pushSprite(&sprites[flip], fontWidth, offset_y, TRANSPARENT);
}

constexpr uint8_t progress[] = {'-', '\\', '|', '/'};

// Connect to wifi
void setupWiFi(void) {
  WiFi.begin("", "");

  // Wait some time to connect to wifi
  for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
    display.setCursor(display.width() >> 1, display.height() >> 1);
    display.printf("%c", progress[(i + 1) % 4]);
    delay(500);
  }

  configTzTime(PSTR("JST-9"), "ntp.nict.jp");
  for (int i = 0; i < 10; i++) {
    display.setCursor(display.width() >> 1, display.height() >> 1);
    display.printf("%c", progress[(i + 1) % 4]);
    delay(500);
  }

  WiFi.disconnect(true);
}

bool bA = false;
bool bB = false;
bool bC = false;

void handler(Button2 &btn) {
  switch (btn.getType()) {
    case clickType::single_click:
      Serial.print("single ");
      bB = true;
      break;
    case clickType::double_click:
      Serial.print("double ");
      bC = true;
      break;
    case clickType::triple_click:
      Serial.print("triple ");
      break;
    case clickType::long_click:
      Serial.print("long ");
      bA = true;
      break;
    case clickType::empty:
      break;
    default:
      break;
  }

  Serial.print("click");
  Serial.print(" (");
  Serial.print(btn.getNumberOfClicks());
  Serial.println(")");
}

bool buttonAPressed(void) {
  bool temp = bA;
  bA        = false;

  return temp;
}

bool buttonBPressed(void) {
  bool temp = bB;
  bB        = false;

  return temp;
}

bool buttonCPressed(void) {
  bool temp = bC;
  bC        = false;

  return temp;
}

void ButtonUpdate() {
  button.loop();
}

void setupButton(void) {
  // G39 button
  button.setClickHandler(handler);
  button.setDoubleClickHandler(handler);
  button.setTripleClickHandler(handler);
  button.setLongClickHandler(handler);
  button.begin(39);

  SDUCfg.setSDUBtnA(&buttonAPressed);
  SDUCfg.setSDUBtnB(&buttonBPressed);
  SDUCfg.setSDUBtnC(&buttonCPressed);
  SDUCfg.setSDUBtnPoller(&ButtonUpdate);
}

void setupSprite(void) {
  if (display.width() < display.height()) {
    display.setRotation(display.getRotation() ^ 1);
  }

  lcd_width  = display.width();
  lcd_height = display.height();

  obj_info_t data;
  for (size_t i = 0; i < obj_count; ++i) {
    data.translate(lcd_width >> 1, lcd_height >> 1);

    data.x      = random(-(lcd_width >> 1), lcd_width >> 1);
    data.y      = random(-(lcd_height >> 1), lcd_height >> 1);
    data.width  = 100;
    data.height = 80;
    data.r      = 0;
    data.z      = random(10, 100);
    data.color  = display.color888(random(255), random(255), random(255));

    data.draw3D();

    obj.push_back(data);
  }

  uint32_t div = 2;
  for (;;) {
    sprite_height = (lcd_height + div - 1) / div;
    bool fail     = false;
    for (std::uint32_t i = 0; !fail && i < 2; ++i) {
      sprites[i].setColorDepth(display.getColorDepth());
      fail = !sprites[i].createSprite(lcd_width, sprite_height);
    }
    if (!fail) break;
    log_e("can't allocate");
    for (std::uint32_t i = 0; i < div; ++i) {
      sprites[i].deleteSprite();
    }
    ++div;
  }

  rectangle.setColorDepth(display.getColorDepth());
  rectangle.setSwapBytes(true);
  rectangle.createSprite(100, 80);

  timeSprite.setColorDepth(display.getColorDepth());
  timeSprite.setSwapBytes(true);
  timeSprite.createSprite(43, 65);
}

uint32_t getColors2(int count) {
  uint32_t p = 0;
  if ((count % 8) & 1) p = 0x03;
  if ((count % 8) & 2) p += 0x1C;
  if ((count % 8) & 4) p += 0xE0;
  return p;
}

uint32_t getColors(int count) {
  int r = 0, g = 0, b = 0;
  switch (count >> 4) {
    case 0:
      b = 255;
      g = count * 0x11;
      break;
    case 1:
      b = 255 - (count & 15) * 0x11;
      g = 255;
      break;
    case 2:
      g = 255;
      r = (count & 15) * 0x11;
      break;
    case 3:
      r = 255;
      g = 255 - (count & 15) * 0x11;
      break;
  }
  return display.color888(r, g, b);
}

void setup(void) {
  display.init();
  display.startWrite();

  setupButton();
  setSDUGfx(&display);
  checkSDUpdater(
      SD,            // filesystem (default=SD)
      MENU_BIN,      // path to binary (default=/menu.bin, empty string=rollback only)
      10000,         // wait delay, (default=0, will be forced to 2000 upon ESP.restart() )
      TFCARD_CS_PIN  // (usually default=4 but your mileage may vary)
  );

  setupWiFi();
  setupSprite();
}

void loop(void) {
  static uint8_t flip      = 0;
  static uint8_t time_flip = 0;

  ButtonUpdate();

  for (int i = 0; i < obj.size(); ++i) {
    obj[i].draw3D();
  }

  std::sort(obj.begin(), obj.end(), std::greater<obj_info_t>());

  for (int_fast16_t y = 0; y < lcd_height; y += sprite_height) {
    flip = flip ? 0 : 1;
    sprites[flip].clear();

    for (size_t i = 0; i < obj.size(); ++i) {
      obj_info_t data = obj[i];
      rectangle.fillRect(0, 0, 100, 80, data.color);
      // rectangle.drawRect(0, 0, 100, 80, TFT_WHITE);
      rectangle.pushRotateZoom(&sprites[flip], data.dx, data.dy - y, data.r, data._scale, data._scale, TRANSPARENT);
    }

    if (y == 0) {
      struct tm timeinfo;
      getLocalTime(&timeinfo);
      drawTime(flip, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

      if ((timeinfo.tm_hour == 12) && (timeinfo.tm_min == 0) && (timeinfo.tm_sec == 0)) {
        ESP.restart();
      }
    }

    sprites[flip].pushSprite(&display, 0, y);
  }

  display.display();
}
