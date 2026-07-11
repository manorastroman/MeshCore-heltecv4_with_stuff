# ESP32-S3 DevKit + SX1262 Room Server

A MeshCore room server built from a bare ESP32-S3-WROOM-1 devkit (N16R8: 16 MB
flash), an E22-style SX1262 LoRa breakout (with RXEN/TXEN pins), and a 1.5"
128x128 I2C OLED (GME128128-01, SSD1327 controller) for status. An optional
DS3231 RTC module can be plugged into the same I2C bus at any time — it is
auto-detected at boot and keeps the clock across power cuts.

Default LoRa parameters are the MeshCore US-915 preset (910.525 MHz, BW 250,
SF 10, CR 5). Change them at runtime via the serial CLI or the phone app.

## Wiring

| SX1262 pin | ESP32-S3 GPIO |
|------------|---------------|
| NSS / CS   | 10 |
| SCK / CLK  | 12 |
| MOSI       | 11 |
| MISO       | 13 |
| RESET      | 14 |
| BUSY       | 9  |
| DIO1       | 8  |
| TXEN       | 15 |
| RXEN       | 16 |
| DIO2       | not connected |
| 3V3 / GND  | 3V3 / GND |

| OLED / DS3231 pin | ESP32-S3 GPIO |
|-------------------|---------------|
| SDA | 17 |
| SCL | 18 |
| VCC / GND | 3V3 / GND |

The BOOT button (GPIO0) wakes the display; it turns itself off again after
about 20 seconds. Avoid repurposing GPIOs 19/20 (USB), 26-37 (flash + octal
PSRAM), 43/44 (UART0), 0/3/45/46 (strapping), and 48 (onboard WS2812 LED).

## Build & flash

```
pio run -e ESP32S3_devkit_SX1262_room_server -t upload
pio device monitor -b 115200
```

The serial console is on the devkit's **native USB** port (`ARDUINO_USB_CDC_ON_BOOT=1`).

## First-boot checklist

- **OLED blank?** Some 128x128 modules use a different init sequence or
  address. Try, in order:
  - `-D DISPLAY_ADDRESS=0x3D` (run an I2C scanner if unsure)
  - `-D U8G2_CONTROLLER=U8G2_SSD1327_MIDAS_128X128_F_HW_I2C`
  - `-D U8G2_CONTROLLER=U8G2_SSD1327_EA_W128128_F_HW_I2C`
  - `-D U8G2_CONTROLLER=U8G2_SH1107_PIMORONI_128X128_F_HW_I2C` (SH1107 panels)
- **`radio init failed` on serial?** Error -706/-707 is retried automatically
  with the TCXO disabled (crystal-only modules). Any other persistent code
  means wiring — recheck NSS(10) and BUSY(9) first.
- **Transmits but never receives?** Suspect the RXEN(16)/TXEN(15) wiring.
- **Clock**: log in as admin from the phone app to set the time, or plug a
  DS3231 into SDA 17 / SCL 18 — it is picked up on the next boot and the time
  then survives reboots.
