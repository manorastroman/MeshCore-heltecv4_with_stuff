# ESP32-S3 DevKit + SX1262 Room Server

A MeshCore room server built from a bare ESP32-S3-WROOM-1 devkit (N16R8: 16 MB
flash), an E22-style SX1262 LoRa breakout (with RXEN/TXEN pins), and a 1.5"
128x128 I2C OLED (GME128128-01, **SH1107** controller — Seeed U8g2 profile) for
status. An optional
DS3231 RTC module can be plugged into the same I2C bus at any time — it is
auto-detected at boot and keeps the clock across power cuts.

Default LoRa parameters (910.252 MHz, BW 62.5, SF 7, CR 5) match the local
US-915 mesh this node joins. Change them at runtime via the serial CLI
(`set radio <freq>,<bw>,<sf>,<cr>`) or the phone app.

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

This is a mains-powered node, so the display is set to stay on permanently
(`-D AUTO_OFF_MILLIS=0`); the BOOT button (GPIO0) is therefore unused for the
display. Set `AUTO_OFF_MILLIS` to a non-zero millisecond value if you'd rather
have the 20-second power-save wake-on-button behaviour back. Avoid repurposing
GPIOs 19/20 (USB), 26-37 (flash + octal
PSRAM), 43/44 (UART0), 0/3/45/46 (strapping), and 48 (onboard WS2812 LED).

## Perfboard build (70 x 90 mm)

Physical layout on the 70x90 mm board from the Elegoo Double Sided PCB Kit.
Coordinates are `row,column`; the devkit occupies rows e-z with its pin headers
in columns `n` and `x`.

### ESP32-S3 devkit placement

| Row | col n | col x |
|-----|-------|-------|
| e | GND | GND |
| f | 5V  | GND |
| g | 14  | 19  |
| h | 13  | 20  |
| i | 12  | 21  |
| j | 11  | 47  |
| k | 10  | 48  |
| l | 9   | 45  |
| m | 46  | 0   |
| n | 3   | 35  |
| o | 8   | 36  |
| p | 18  | 37  |
| q | 17  | 38  |
| r | 16  | 39  |
| s | 15  | 40  |
| t | 7   | 41  |
| u | 6   | 42  |
| v | 5   | 2   |
| w | 4   | 1   |
| x | RST | RX  |
| y | 3V3 | TX  |
| z | 3V3 | GND |

### SX1262 breakout

| Src pin | Name  | Dest pin | Net |
|---------|-------|----------|-----|
| c,c | BUSY  | l,n | GPIO 9  |
| c,d | RESET | g,n | GPIO 14 |
| c,e | MISO  | h,n | GPIO 13 |
| c,f | MOSI  | j,n | GPIO 11 |
| c,g | CLK   | i,n | GPIO 12 |
| c,h | CS    | k,n | GPIO 10 |
| c,i | GND   | f,x | GND |
| c,j | ANT   | n/c | (RF via the module's antenna connector) |
| i,c | 3V3   | y,n | 3V3 |
| i,d | GND   | f,x | GND |
| i,e | DIO1  | o,n | GPIO 8  |
| i,f | DIO2  | n/c | — |
| i,g | TXEN  | s,n | GPIO 15 |
| i,h | RXEN  | r,n | GPIO 16 |
| i,i | GND   | f,x | GND |
| i,j | GND   | f,x | GND |

Never transmit without an antenna attached — it can damage the radio's PA.

### DS3231 RTC (OLED daisy-chains off its passthrough header)

| Src pin | Name | Dest pin | Net |
|---------|------|----------|-----|
| l,d | GND | f,x | GND |
| l,e | VCC | y,n | 3V3 |
| l,f | SDA | q,n | GPIO 17 |
| l,g | SCL | p,n | GPIO 18 |
| l,h | SQW | n/c | — |
| l,i | 32K | n/c | — |

The GME128128 OLED plugs into the DS3231's passthrough header, so it shares
the same I2C wiring (addresses don't clash: OLED 0x3C, DS3231 0x68).

### Case buttons

The devkit's own buttons are inaccessible inside the enclosure, so both are
duplicated on the case — momentary normally-open switches to GND, no external
resistors needed (both lines have pull-ups and sit in parallel with the
onboard buttons):

| Button | From | To |
|--------|------|-----|
| Display wake (BOOT / GPIO0) | m,x | GND |
| Reset (RST / EN) | x,n | GND |

GPIO0 is a strapping pin: held low through a reset or power-on, the S3 enters
the USB bootloader instead of booting the firmware. Don't press the wake
button while plugging in power — and use it deliberately (hold wake, tap
reset, release) to reflash over USB without opening the case.

### Power distribution

3V3 (y,n) feeds the radio, RTC, and OLED; GND collects at f,x. With single
plated holes rather than strips, run a short 3V3/GND bus near the modules and
jump to the devkit pins once instead of stacking every wire on one pad.

### Mains supply (24/7 operation)

Powered from a **7.5 V / 500 mA** wall supply through a buck converter (module
rated 3.2-35 V in, 3 A out) set to **5 V**, feeding the devkit's **5V/VIN pin**
(let the onboard regulator make its own 3.3 V — do not inject 5 V into 3V3).

Budget check: the node draws ~90-120 mA at 5 V idle/RX and ~215 mA at SX1262
TX peak (+22 dBm). Reflected through the buck (~90% eff.) that's only ~75 mA
idle and ~160 mA peak drawn from the 7.5 V supply — comfortably under its
500 mA rating (~3x headroom). The buck's 3 A capacity is never the limit.

Add a bulk cap (~470 uF) on the buck's 5 V output near the board: the SX1262's
short TX current spikes can otherwise sag the rail and trigger brownouts or
radio init errors (-706/-707).

## Build & flash

```
pio run -e ESP32S3_devkit_SX1262_room_server -t upload
pio device monitor -b 115200
```

The serial console is on the devkit's **native USB** port (`ARDUINO_USB_CDC_ON_BOOT=1`).

## First-boot checklist

- **OLED shows snow / static?** The panel is alive and addressed, but the
  controller family is wrong. The GME128128-01 is an **SH1107**, not an SSD1327
  (verified against a known-good driver for this exact panel). The working
  config is `-D U8G2_CONTROLLER=U8G2_SH1107_SEEED_128X128_F_HW_I2C`.
- **Image legible but shifted ~1/4 screen left?** Right controller, wrong
  x-offset. The generic `U8G2_SH1107_128X128` profile uses `x_offset=96`; this
  panel needs `0`, which is what the `_SEEED_` (or `_PIMORONI_`) profile sets.
- **OLED totally blank?** Wrong I2C address — try `-D DISPLAY_ADDRESS=0x3D`
  (this panel answers at 0x3C; run an I2C scanner if unsure).
- **`radio init failed` on serial?** Error -706/-707 is retried automatically
  with the TCXO disabled (crystal-only modules). Any other persistent code
  means wiring — recheck NSS(10) and BUSY(9) first.
- **Transmits but never receives?** Suspect the RXEN(16)/TXEN(15) wiring.
- **Clock**: log in as admin from the phone app to set the time, or plug a
  DS3231 into SDA 17 / SCL 18 — it is picked up on the next boot and the time
  then survives reboots.
