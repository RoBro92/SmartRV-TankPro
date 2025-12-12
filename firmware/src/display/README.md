# Firmware – Display (Cheap Yellow Display LVGL)

LVGL 9.x firmware for the 2.8" **Cheap Yellow Display (ESP32-2432S028)**. The UI mirrors TankPro concepts (fresh/waste tanks, faults, display settings) and now drives live controller data plus commands (fill, drain, clear faults, restart, config).

## Layout
- `CYD/`: PlatformIO project targeting the ESP32-2432S028 with ILI9341 TFT + XPT2046 touch.
  - `platformio.ini`: `env:cyd` build target; pulls LVGL and LovyanGFX.
  - `main.cpp`: LVGL bring-up, touch + brightness handling, sleep timeout.
  - `ui/`: SquareLine-generated LVGL UI (v0.1.0 label baked into boot/settings).
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
- Current build: live Wi‑Fi link to TankPro controllers, pairing/assignment, config sync, Clear Faults, Fill/Drain, and Restart commands.
- Next: tighter direct-mode support without infrastructure Wi‑Fi plus richer on-screen diagnostics.

See `docs/display-firmware.md` for project details and `docs/display-firmware-installation.md` for end-user flashing and update instructions.
