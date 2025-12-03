# Display Firmware (Cheap Yellow Display)

LVGL-based demo firmware for the 2.8" **Cheap Yellow Display (ESP32-2432S028)**. This build targets the stock ILI9341 TFT + XPT2046 touch stack and shows a TankPro-themed UI (home, fresh/waste detail, faults, and display settings). Current build label: **v0.0.1**.

For a step-by-step flashing guide (PlatformIO and esptool.py) aimed at users buying their own CYD, see `docs/display-firmware-installation.md`.

> Status: UI/demo only right now. It does not yet talk to the TankPro controller over Wi‑Fi or UART; values on screen are placeholders. The home screen buttons and Wi‑Fi/Direct selectors are for future transport selection.

## Files and outputs
- Source: `firmware/src/display/CYD/` (PlatformIO project).
- Build target: `env:cyd` (Arduino on ESP32).
- LVGL config: `lv_conf.h`.
- Build artifacts (after `pio run`): `firmware/src/display/CYD/.pio/build/cyd/firmware.bin`, plus `bootloader.bin` and `partitions.bin`.

## Hardware required
- Cheap Yellow Display / ESP32-2432S028 (240×320 ILI9341 + XPT2046 touch).
- USB-C or Micro-USB cable for flashing (depends on your board revision).
- USB-to-UART driver for your OS (CH9102/CP2102/CH340 as fitted to your board).

## Flash with PlatformIO (recommended)
```bash
cd firmware/src/display/CYD
pio run -e cyd                        # build LVGL firmware
pio run -t upload -e cyd --upload-port <port>   # flash (e.g., /dev/cu.usbserial-XXXXX or COM5)
```
- If the board doesn’t auto-reset into bootloader: hold the `BOOT` button, tap `RST`, then release `BOOT` once upload starts.
- Power the display from USB while flashing. For bench tests you can also feed 5 V/GND to the header.

## Flash with esptool.py (using the built binaries)
1) Build once with PlatformIO so the `.pio/build/cyd` binaries exist (or use provided artifacts if shipped with your checkout).  
2) Run:
```bash
cd firmware/src/display/CYD/.pio/build/cyd
esptool.py --chip esp32 --baud 460800 --before default_reset --after no_reset write_flash -z \
  0x1000  bootloader.bin \
  0x8000  partitions.bin \
  0x10000 firmware.bin
```
- Adjust baud or serial port with `--port <device>` if needed.

## What you’ll see on first boot
- Boot screen offering **Wi‑Fi** or **Direct** (non-functional placeholders for now).
- Home cards for **Fresh** and **Waste** tanks with sample percentages and temps.
- A **Settings** screen with brightness slider, display sleep timeout, light/dark theme, and firmware label `v0.0.1`.

## Updating
- Rebuild and re-flash using either method above. Brightness, sleep timeout, and theme selections persist on the display between updates; the UI remains a data-only demo until controller integration lands.

## Next steps (planned)
- Wire the Wi‑Fi/Direct choices into real connectivity to the TankPro controller.
- Replace placeholder tank values with live data from the controller.
- Ship signed/prebuilt binaries alongside source for easier end-user installs.
