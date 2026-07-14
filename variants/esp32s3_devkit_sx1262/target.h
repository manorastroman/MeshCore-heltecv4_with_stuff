#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <helpers/ESP32Board.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>
#ifdef DISPLAY_CLASS
  #include <helpers/ui/U8g2Display.h>
#endif

class S3DevkitBoard : public ESP32Board {
public:
  void begin() {
    ESP32Board::begin();
  #ifdef PIN_USER_BTN
    pinMode(PIN_USER_BTN, INPUT_PULLUP);
  #endif
  }

  const char* getManufacturerName() const override {
    return "ESP32-S3 DevKit (SX1262)";
  }
};

extern S3DevkitBoard board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern SensorManager sensors;

#ifdef DISPLAY_CLASS
  extern DISPLAY_CLASS display;
#endif

bool radio_init();
mesh::LocalIdentity radio_new_identity();
