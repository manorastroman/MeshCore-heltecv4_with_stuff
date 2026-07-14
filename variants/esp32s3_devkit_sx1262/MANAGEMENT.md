# S3 Room Server — Management & Configuration

How to provision and run this room server once it boots. There are **two
management channels**; the everyday one is your phone.

## 1. Remote admin over the mesh (normal operation)

A room server has no BLE/WiFi of its own — you administer it *through the mesh*
from the **MeshCore companion app** (phone paired to any companion node in range):

1. The room server appears as a contact once it sends its advert (name
   **`S3 Room Server`**).
2. Tap it → **log in as admin** with the admin password.
3. Once authenticated you get the same commands as the serial CLI, and the app
   **auto-sends `clock sync`**, setting the node's clock from your phone
   (`setCurrentTime(sender_timestamp + 1)`). This is the intended timekeeping
   path — no NTP, no DS3231 required.

Guests join the room itself (to read/post messages) with the **room/guest
password**, a separate and lower privilege than admin.

## 2. Serial CLI (bench / direct)

Plug in USB and type commands at **115200 baud** on the devkit's native USB
port. Each line is piped straight into `handleCommand`. Use this to provision
before the node is on the mesh, or to recover it.

> Note: over serial there's no sender timestamp, so `clock sync` won't set the
> time. Use `time <unix-epoch>` instead.

## ⚠️ Change the baked-in defaults first

These are compiled in as **placeholders**:

| Setting            | Current value     |
| ------------------ | ----------------- |
| Admin password     | `password`        |
| Room (guest) password | `hello`        |
| Node name          | `S3 Room Server`  |

You do **not** need to recompile to change them — set them at runtime and they
persist to flash:

```
password <new-admin-pw>            # change admin password
set guest.password <new-room-pw>   # change room/guest password
set name Basement Room Server
```

## Command cheat-sheet (serial or remote-admin)

| Task                | Command                                                            |
| ------------------- | ----------------------------------------------------------------- |
| Set time (serial)   | `time <unix-epoch>`                                                |
| Set time (remote)   | `clock sync` (app does it automatically)                          |
| Show clock          | `clock`                                                            |
| Radio (freq,bw,sf,cr) | `set radio 910.252,62.5,7,5` (comma-separated; reboot to apply) |
| TX power            | `set tx 22`                                                       |
| Frequency only      | `set freq 910.252` (serial only — gated to sender_timestamp 0)    |
| Region              | `region`                                                          |
| Node name           | `set name <text>`                                                 |
| Advert interval     | `set advert.interval <mins>` · manual advert: `advert`           |
| Stats / neighbours  | `get ...` · `neighbors` · `clear stats`                          |
| Version / build     | `ver`                                                            |
| Reboot              | `reboot`                                                         |
| Firmware update     | `start ota`                                                      |

Radio params are already correct from the build flags (910.252 / BW 62.5 /
SF 7 / CR 5 / 22 dBm) and become the persisted defaults on first boot, so
`set freq` etc. are only for later changes.

## What persists

Config (name, passwords, radio, clock offset) and the node identity all live in
flash (`store` / LittleFS), so a reboot or power-cycle keeps everything. The
node identity (Room ID) is permanent unless you `erase`.
