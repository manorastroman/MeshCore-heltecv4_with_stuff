# MeshCore Room Server: ESP32-S3 N16R8 devkit + SX1262 + 128x128 OLED

## Context

Goal: a stationary MeshCore **room server** (US 915) built from hardware on hand. The original idea (CYD display board) was dropped — for a room server the touchscreen adds nothing and its pin scarcity forced ugly wiring. Final hardware, per the user's board vault:

- **ESP32-S3-WROOM-1 N16R8 devkit** (available; 16MB flash, 8MB PSRAM, native USB)
- **SX1262 LoRa breakout** (E22-style: CS/CLK/MOSI/MISO/RESET/BUSY/DIO1/DIO2/RXEN/TXEN)
- **GME128128-01 1.5" 128x128 I2C OLED** (SSD1327 controller) for a simple stats display
- Optional plug-in: **DS3231 RTC** on the same I2C bus (auto-detected by existing `AutoDiscoverRTCClock`, zero code) — time is otherwise set via phone-app admin login (no NTP exists in MeshCore, none added)
- No GPS (stationary)

Repo is pristine MeshCore v1.16.0; branch `claude/hardware-planning-y0jjfv` (currently identical to main). Primary template: `variants/rak3112/` (ESP32-S3 + SX1262 on `esp32-s3-devkitc-1`). Display pattern: `variants/lilygo_techo_card/` (U8g2Display + `olikraus/U8g2` lib). Room server (`examples/simple_room_server/`) has display support via its own `UITask`; it wakes the display with `digitalRead(PIN_USER_BTN) == USER_BTN_PRESSED` and auto-offs after ~20s.

## Hardware wiring (goes in variant README)

Radio (all plain header pins, no soldering hacks):
| SX1262 pin | S3 GPIO | | Other | S3 GPIO |
|---|---|---|---|---|
| NSS/CS | 10 | | OLED SDA (+DS3231 SDA) | 17 |
| SCK | 12 | | OLED SCL (+DS3231 SCL) | 18 |
| MOSI | 11 | | Display wake button | BOOT (GPIO0) |
| MISO | 13 | | | |
| RESET | 14 | | | |
| BUSY | 9 | | | |
| DIO1 | 8 | | | |
| TXEN | 15 | | | |
| RXEN | 16 | | | |
| DIO2 | not connected | | | |

3V3/GND for radio + OLED from the devkit. I2C addresses: OLED 0x3C, DS3231 0x68 — no conflict, RTC is plug-in-anytime.
Avoided pins: 19/20 (USB), 26–37 (flash + octal PSRAM), 43/44 (UART0), 0/3/45/46 (straps), 48 (WS2812).

## Changes

### 1. `src/helpers/ui/U8g2Display.h` — make panel constructor overridable (only shared-file edit)
The class hardcodes `U8G2_SSD1306_72X40_ER_F_HW_I2C _u8g2;` (line 22). Add a default-preserving macro:
```cpp
#ifndef U8G2_CONTROLLER
  #define U8G2_CONTROLLER U8G2_SSD1306_72X40_ER_F_HW_I2C
#endif
...
U8G2_CONTROLLER _u8g2;
```
Zero behavioral change for the existing consumer (`lilygo_techo_card`). `OLED_WIDTH`/`OLED_HEIGHT`/`DISPLAY_ADDRESS` are already overridable macros. Note: `begin()` relies on Wire being initialized by `board.begin()` — satisfied via `PIN_BOARD_SDA/SCL` below.

### 2. `variants/esp32s3_devkit_sx1262/target.h` (new)
Modeled on `variants/generic-e22/target.h` + rak3112:
- `#define RADIOLIB_STATIC_ONLY 1`; includes RadioLib, `helpers/radiolib/RadioLibWrappers.h`, `helpers/radiolib/CustomSX1262Wrapper.h`, `helpers/ESP32Board.h`, `helpers/AutoDiscoverRTCClock.h`, `helpers/SensorManager.h`; `#ifdef DISPLAY_CLASS` → `<helpers/ui/U8g2Display.h>`.
- `class S3DevkitBoard : public ESP32Board` — `begin()` calls `ESP32Board::begin()` then `pinMode(PIN_USER_BTN, INPUT_PULLUP)` (room server UITask raw-reads the pin, nothing else configures it); `getManufacturerName()` → `"ESP32-S3 DevKit (SX1262)"`.
- Externs: `S3DevkitBoard board;` `WRAPPER_CLASS radio_driver;` `AutoDiscoverRTCClock rtc_clock;` `SensorManager sensors;` `DISPLAY_CLASS display;` (guarded); `bool radio_init(); mesh::LocalIdentity radio_new_identity();`

### 3. `variants/esp32s3_devkit_sx1262/target.cpp` (new)
heltec_v4 pattern (`variants/heltec_v4/target.cpp` lines 12–36):
- `S3DevkitBoard board;`
- `static SPIClass spi;` (S3 default = FSPI — no conflict, nothing else uses SPI; `radio.std_init(&spi)` does `spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI)` per `src/helpers/radiolib/CustomSX1262.h`)
- `RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);` + `WRAPPER_CLASS radio_driver(radio, board);`
- `ESP32RTCClock fallback_clock; AutoDiscoverRTCClock rtc_clock(fallback_clock);` — DS3231 optional plug-in
- `SensorManager sensors;` (concrete base, no-op — no GPS/telemetry)
- `#ifdef DISPLAY_CLASS DISPLAY_CLASS display; #endif`
- `radio_init()`: `fallback_clock.begin(); rtc_clock.begin(Wire); return radio.std_init(&spi);`
- `radio_new_identity()` via `RadioNoiseListener rng(radio)`

### 4. `variants/esp32s3_devkit_sx1262/platformio.ini` (new — auto-globbed by root config)
```ini
[ESP32S3_devkit_SX1262]              ; abstract base
extends = esp32_base
board = esp32-s3-devkitc-1
board_upload.flash_size = 16MB
board_build.partitions = default_16MB.csv
build_flags =
  ${esp32_base.build_flags}
  -I variants/esp32s3_devkit_sx1262
  -D ESP32S3_DEVKIT_SX1262
  -D ARDUINO_USB_CDC_ON_BOOT=1       ; console on the native-USB port
  -D USE_SX1262
  -D RADIO_CLASS=CustomSX1262
  -D WRAPPER_CLASS=CustomSX1262Wrapper
  -D P_LORA_NSS=10 -D P_LORA_SCLK=12 -D P_LORA_MOSI=11 -D P_LORA_MISO=13
  -D P_LORA_RESET=14 -D P_LORA_BUSY=9 -D P_LORA_DIO_1=8
  -D SX126X_TXEN=15                  ; firmware drives the RF switch (generic-e22 pattern)
  -D SX126X_RXEN=16
  -D SX126X_DIO2_AS_RF_SWITCH=true   ; harmless with DIO2 unconnected; matches generic-e22
  -D SX126X_DIO3_TCXO_VOLTAGE=1.8    ; std_init auto-retries with 0.0 on -706/-707 (XTAL modules)
  -D SX126X_CURRENT_LIMIT=140
  -D SX126X_RX_BOOSTED_GAIN=1
  -D PIN_BOARD_SDA=17 -D PIN_BOARD_SCL=18   ; OLED + optional DS3231
  -D PIN_USER_BTN=0                  ; BOOT button wakes display (pressed = LOW)
build_src_filter = ${esp32_base.build_src_filter}
  +<../variants/esp32s3_devkit_sx1262>
lib_deps = ${esp32_base.lib_deps}

[env:ESP32S3_devkit_SX1262_room_server]
extends = ESP32S3_devkit_SX1262
build_src_filter = ${ESP32S3_devkit_SX1262.build_src_filter}
  +<helpers/ui/U8g2Display.h>
  +<../examples/simple_room_server>
build_flags =
  ${ESP32S3_devkit_SX1262.build_flags}
  -D DISPLAY_CLASS=U8g2Display
  -D U8G2_CONTROLLER=U8G2_SSD1327_WS_128X128_F_HW_I2C   ; GME128128-01 = SSD1327; alt ctors in README
  -D OLED_WIDTH=128
  -D OLED_HEIGHT=128
  ; US-915 defaults (override arduino_base's EU 869.618/62.5/8; last -D wins)
  -D LORA_FREQ=910.525
  -D LORA_BW=250
  -D LORA_SF=10
  -D LORA_CR=5
  -D LORA_TX_POWER=22
  -D ADVERT_NAME='"S3 Room Server"'
  -D ADVERT_LAT=0.0
  -D ADVERT_LON=0.0
  -D ADMIN_PASSWORD='"password"'
  -D ROOM_PASSWORD='"hello"'
lib_deps =
  ${ESP32S3_devkit_SX1262.lib_deps}
  ${esp32_ota.lib_deps}
  olikraus/U8g2 @ ^2.35.19           ; same pin as lilygo_techo_card
```
A future `[env:ESP32S3_devkit_SX1262_repeater]` or companion env follows the same pattern (repeater adds `+<../examples/simple_repeater>`, `-D MAX_NEIGHBOURS=50`, `bakercp/CRC32 @ ^2.0.0`).

### 5. `variants/esp32s3_devkit_sx1262/README.md` (new)
Wiring table above, OLED constructor alternates, flash/verify instructions.

## Key reference files while implementing
- `variants/rak3112/platformio.ini` — S3 devkit + SX1262 env structure
- `variants/heltec_v4/target.cpp` — AutoDiscoverRTCClock + radio instantiation pattern
- `variants/generic-e22/target.{h,cpp}` — minimal target shape, RXEN flags
- `variants/lilygo_techo_card/platformio.ini` — U8g2Display env pattern
- `src/helpers/ui/U8g2Display.h` — the one shared file edited
- `src/helpers/radiolib/CustomSX1262.h` — `std_init` SPI/TCXO behavior

## Verification
1. `pio run -e ESP32S3_devkit_SX1262_room_server` — clean build; also `pio run -e techo-card_companion_radio_usb` (or whichever env consumes U8g2Display) to prove the shared-header change is behavior-neutral. Confirm flash fits.
2. Commit and push to `claude/hardware-planning-y0jjfv`.
3. Flash-time checklist (in README — can't be tested here):
   - OLED shows MeshCore boot screen then room stats. If blank, try alternate U8g2 constructors: `U8G2_SSD1327_MIDAS_128X128_F_HW_I2C`, `U8G2_SSD1327_EA_W128128_F_HW_I2C` (or `U8G2_SH1107_PIMORONI_128X128_F_HW_I2C` if the panel is actually SH1107); check address with an I2C scan (0x3C vs 0x3D → `-D DISPLAY_ADDRESS=0x3D`).
   - Serial console on the USB port (CDC): watch for `radio init failed`; -706/-707 auto-recovers (XTAL module).
   - TX: phone/another node sees the boot advert; RX: post to the room from the app. Deaf RX → RXEN wiring/polarity suspect.
   - BOOT button wakes the OLED; it sleeps again after ~20 s.
   - Set clock via phone-app admin login; optionally plug DS3231 into 17/18 — auto-detected at next boot, keeps time thereafter.