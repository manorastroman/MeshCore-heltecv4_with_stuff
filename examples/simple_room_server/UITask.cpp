#include "UITask.h"
#include "MyMesh.h"
#include <Arduino.h>
#include <time.h>
#include <helpers/CommonCLI.h>

#ifndef USER_BTN_PRESSED
#define USER_BTN_PRESSED LOW
#endif

// ---- 12x12 status icons (XBM, LSB-first) --------------------------------
static const uint8_t icon_signal [] PROGMEM = {   // rising signal bars -> radio
  0x00,0x06, 0x00,0x06, 0x00,0x06, 0xC0,0x06, 0xC0,0x06, 0xC0,0x06,
  0xD8,0x06, 0xD8,0x06, 0xD8,0x06, 0xDB,0x06, 0xDB,0x06, 0xDB,0x06 };
static const uint8_t icon_people [] PROGMEM = {   // person -> clients
  0x00,0x00, 0xF0,0x00, 0xF0,0x00, 0xF0,0x00, 0x00,0x00, 0xF8,0x01,
  0xFC,0x03, 0xFC,0x03, 0xFC,0x03, 0xFC,0x03, 0xFC,0x03, 0x00,0x00 };
static const uint8_t icon_mail [] PROGMEM = {     // envelope -> messages
  0x00,0x00, 0xFF,0x07, 0x01,0x04, 0x03,0x06, 0x8D,0x05, 0x71,0x04,
  0x01,0x04, 0x01,0x04, 0x01,0x04, 0x01,0x04, 0xFF,0x07, 0x00,0x00 };
static const uint8_t icon_clock [] PROGMEM = {    // clock -> uptime
  0xF0,0x00, 0x0C,0x03, 0x02,0x04, 0x22,0x04, 0x21,0x08, 0xE1,0x08,
  0x01,0x08, 0x02,0x04, 0x02,0x04, 0x0C,0x03, 0xF0,0x00, 0x00,0x00 };

static const char* const MONTHS[] = {
  "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

#ifndef AUTO_OFF_MILLIS
#define AUTO_OFF_MILLIS      20000  // 20 seconds (0 = never auto-off; stays on for mains-powered nodes)
#endif
#define BOOT_SCREEN_MILLIS   4000   // 4 seconds

// 'meshcore', 128x13px
static const uint8_t meshcore_logo [] PROGMEM = {
    0x3c, 0x01, 0xe3, 0xff, 0xc7, 0xff, 0x8f, 0x03, 0x87, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 
    0x3c, 0x03, 0xe3, 0xff, 0xc7, 0xff, 0x8e, 0x03, 0x8f, 0xfe, 0x3f, 0xfe, 0x1f, 0xff, 0x1f, 0xfe, 
    0x3e, 0x03, 0xc3, 0xff, 0x8f, 0xff, 0x0e, 0x07, 0x8f, 0xfe, 0x7f, 0xfe, 0x1f, 0xff, 0x1f, 0xfc, 
    0x3e, 0x07, 0xc7, 0x80, 0x0e, 0x00, 0x0e, 0x07, 0x9e, 0x00, 0x78, 0x0e, 0x3c, 0x0f, 0x1c, 0x00, 
    0x3e, 0x0f, 0xc7, 0x80, 0x1e, 0x00, 0x0e, 0x07, 0x1e, 0x00, 0x70, 0x0e, 0x38, 0x0f, 0x3c, 0x00, 
    0x7f, 0x0f, 0xc7, 0xfe, 0x1f, 0xfc, 0x1f, 0xff, 0x1c, 0x00, 0x70, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x1f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x0e, 0x38, 0x0e, 0x3f, 0xf8, 
    0x7f, 0x3f, 0xc7, 0xfe, 0x0f, 0xff, 0x1f, 0xff, 0x1c, 0x00, 0xf0, 0x1e, 0x3f, 0xfe, 0x3f, 0xf0, 
    0x77, 0x3b, 0x87, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xfc, 0x38, 0x00, 
    0x77, 0xfb, 0x8f, 0x00, 0x00, 0x07, 0x1c, 0x0f, 0x3c, 0x00, 0xe0, 0x1c, 0x7f, 0xf8, 0x38, 0x00, 
    0x73, 0xf3, 0x8f, 0xff, 0x0f, 0xff, 0x1c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x78, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfe, 0x3c, 0x0e, 0x3f, 0xf8, 0xff, 0xfc, 0x70, 0x3c, 0x7f, 0xf8, 
    0xe3, 0xe3, 0x8f, 0xff, 0x1f, 0xfc, 0x3c, 0x0e, 0x1f, 0xf8, 0xff, 0xf8, 0x70, 0x3c, 0x7f, 0xf8, 
};

void UITask::begin(MyMesh* mesh, NodePrefs* node_prefs, const char* build_date, const char* firmware_version) {
  _prevBtnState = HIGH;
  _auto_off = millis() + AUTO_OFF_MILLIS;
  _mesh = mesh;
  _node_prefs = node_prefs;
  _display->turnOn();

  // strip off dash and commit hash by changing dash to null terminator
  // e.g: v1.2.3-abcdef -> v1.2.3
  char *version = strdup(firmware_version);
  char *dash = strchr(version, '-');
  if(dash){
    *dash = 0;
  }

  // v1.2.3 (1 Jan 2025)
  sprintf(_version_info, "%s (%s)", version, build_date);
}

void UITask::renderCurrScreen() {
  char tmp[80];
  if (millis() < BOOT_SCREEN_MILLIS) { // boot screen
    // meshcore logo
    _display->setColor(DisplayDriver::BLUE);
    int logoWidth = 128;
    _display->drawXbm((_display->width() - logoWidth) / 2, 3, meshcore_logo, logoWidth, 13);

    // meshcore website
    const char* website = "https://meshcore.io";
    _display->setColor(DisplayDriver::LIGHT);
    _display->setTextSize(1);
    uint16_t websiteWidth = _display->getTextWidth(website);
    _display->setCursor((_display->width() - websiteWidth) / 2, 22);
    _display->print(website);

    // version info
    _display->setColor(DisplayDriver::LIGHT);
    _display->setTextSize(1);
    uint16_t versionWidth = _display->getTextWidth(_version_info);
    _display->setCursor((_display->width() - versionWidth) / 2, 35);
    _display->print(_version_info);

    // node type
    const char* node_type = "< Room Server >";
    uint16_t typeWidth = _display->getTextWidth(node_type);
    _display->setCursor((_display->width() - typeWidth) / 2, 48);
    _display->print(node_type);
  } else if (_mesh != NULL && _display->height() >= 128) {  // rich status card on large panels
    renderStatusCard();
  } else {  // compact home screen (small panels)
    // node name
    _display->setCursor(0, 0);
    _display->setTextSize(1);
    _display->setColor(DisplayDriver::GREEN);
    _display->print(_node_prefs->node_name);

    // freq / sf
    _display->setCursor(0, 20);
    _display->setColor(DisplayDriver::YELLOW);
    sprintf(tmp, "FREQ: %06.3f SF%d", _node_prefs->freq, _node_prefs->sf);
    _display->print(tmp);

    // bw / cr
    _display->setCursor(0, 30);
    sprintf(tmp, "BW: %03.2f CR: %d", _node_prefs->bw, _node_prefs->cr);
    _display->print(tmp);
  }
}

// A designed status card for 128x128 (SH1107/SSD1327-class) panels:
// inverted title bar, icon-labelled stat rows, dividers, and a large clock.
void UITask::renderStatusCard() {
  char tmp[40];
  const int W = _display->width();

  // --- title bar: node name, black-on-white ---
  _display->setColor(DisplayDriver::LIGHT);
  _display->fillRect(0, 0, W, 15);
  _display->setColor(DisplayDriver::DARK);   // draw "off" pixels -> inverted text
  _display->setTextSize(2);
  _display->drawTextCentered(W / 2, 3, _node_prefs->node_name);

  _display->setColor(DisplayDriver::LIGHT);

  // --- radio: signal icon + freq + mode ---
  _display->drawXbm(3, 20, icon_signal, 12, 12);
  _display->setTextSize(1);
  sprintf(tmp, "%.3f MHz", _node_prefs->freq);
  _display->setCursor(20, 20);
  _display->print(tmp);
  sprintf(tmp, "BW%g SF%d CR%d", _node_prefs->bw, _node_prefs->sf, _node_prefs->cr);
  _display->setCursor(20, 30);
  _display->print(tmp);

  // --- divider ---
  _display->fillRect(3, 42, W - 6, 1);

  // --- clients | messages: icon + big number ---
  _display->drawXbm(3, 48, icon_people, 12, 12);
  _display->setTextSize(2);
  sprintf(tmp, "%d", _mesh->getNumClients());
  _display->setCursor(20, 49);
  _display->print(tmp);
  _display->setTextSize(1);
  _display->setCursor(20, 62);
  _display->print("clients");

  _display->drawXbm(W / 2 + 3, 48, icon_mail, 12, 12);
  _display->setTextSize(2);
  sprintf(tmp, "%u", (unsigned)_mesh->getNumPosted());
  _display->setCursor(W / 2 + 20, 49);
  _display->print(tmp);
  _display->setTextSize(1);
  _display->setCursor(W / 2 + 20, 62);
  _display->print("msgs");

  // --- uptime ---
  _display->drawXbm(3, 72, icon_clock, 12, 12);
  uint32_t up = _mesh->getUptimeSecs();
  uint32_t d = up / 86400, h = (up % 86400) / 3600, m = (up % 3600) / 60;
  if (d > 0) sprintf(tmp, "up %lud %02lu:%02lu", (unsigned long)d, (unsigned long)h, (unsigned long)m);
  else       sprintf(tmp, "up %02lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)(up % 60));
  _display->setTextSize(1);
  _display->setCursor(20, 75);
  _display->print(tmp);

  // --- divider ---
  _display->fillRect(3, 88, W - 6, 1);

  // --- big clock + date (phone-synced RTC) ---
  time_t now = (time_t)_mesh->getRTCClock()->getCurrentTime();
  struct tm t;
  gmtime_r(&now, &t);
  _display->setTextSize(3);
  sprintf(tmp, "%02d:%02d", t.tm_hour, t.tm_min);
  _display->drawTextCentered(W / 2, 94, tmp);
  _display->setTextSize(1);
  sprintf(tmp, "%d %s %d", t.tm_mday, MONTHS[t.tm_mon % 12], t.tm_year + 1900);
  _display->drawTextCentered(W / 2, 116, tmp);
}

void UITask::loop() {
#ifdef PIN_USER_BTN
  if (millis() >= _next_read) {
    int btnState = digitalRead(PIN_USER_BTN);
    if (btnState != _prevBtnState) {
      if (btnState == USER_BTN_PRESSED) {  // pressed?
        if (_display->isOn()) {
          // TODO: any action ?
        } else {
          _display->turnOn();
        }
        _auto_off = millis() + AUTO_OFF_MILLIS;   // extend auto-off timer
      }
      _prevBtnState = btnState;
    }
    _next_read = millis() + 200;  // 5 reads per second
  }
#endif

  if (_display->isOn()) {
    if (millis() >= _next_refresh) {
      _display->startFrame();
      renderCurrScreen();
      _display->endFrame();

      _next_refresh = millis() + 1000;   // refresh every second
    }
#if AUTO_OFF_MILLIS > 0
    if (millis() > _auto_off) {
      _display->turnOff();
    }
#endif
  }
}
