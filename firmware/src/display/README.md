# Firmware – Display (Cheap Yellow Display LVGL)

LVGL 9.x demo firmware for the 2.8" **Cheap Yellow Display (ESP32-2432S028)**. The UI mirrors TankPro concepts (fresh/waste tanks, fault views, display settings) and currently runs with placeholder data while connectivity is built out.

## Layout
- `CYD/`: PlatformIO project targeting the ESP32-2432S028 with ILI9341 TFT + XPT2046 touch.
  - `platformio.ini`: `env:cyd` build target; pulls LVGL and LovyanGFX.
  - `main.cpp`: LVGL bring-up, touch + brightness handling, sleep timeout.
  - `ui/`: SquareLine-generated LVGL UI (v0.0.1 label baked into boot/settings).
  - Build outputs land in `.pio/build/cyd/` (firmware.bin, bootloader.bin, partitions.bin).

## Flashing
- Recommended: `pio run -e cyd` then `pio run -t upload -e cyd --upload-port <port>` from inside `CYD/`.
- Or flash the built binaries with `esptool.py` (addresses: 0x1000 bootloader, 0x8000 partitions, 0x10000 firmware).
- If the board does not auto-enter bootloader, hold `BOOT`, tap `RST`, then release `BOOT` after upload starts.

## For users buying their own Cheap Yellow Display
- The ready-to-flash binaries are in `.pio/build/cyd/` after a build (`firmware.bin`, `bootloader.bin`, `partitions.bin`).
- Step-by-step install/update instructions (PlatformIO and esptool.py) are in `docs/display-firmware-installation.md`.
- Display settings (brightness, sleep timeout, theme) are persisted locally on the display between reboots.

## Status / roadmap
- UI-only preview; Wi‑Fi/Direct buttons and tank values are placeholders until controller integration is finished.
- Future builds will fetch live data from the TankPro controller over Wi‑Fi or UART.

See `docs/display-firmware.md` for project details and `docs/display-firmware-installation.md` for end-user flashing and update instructions.
