# Display Firmware (Cheap Yellow Display)

LVGL firmware for the 2.8" **Cheap Yellow Display (ESP32-2432S028)**. This build targets the stock ILI9341 TFT + XPT2046 touch stack and shows live TankPro data (fresh/waste), lets you pair controllers, trigger fill/drain, clear faults, tweak thresholds, and restart controllers. Current build label: **v0.2.0**.

For a step-by-step flashing guide (PlatformIO and esptool.py) aimed at users buying their own CYD, see `docs/display-firmware-installation.md`.

## Files and outputs
- Source: `firmware/src/display/CYD/` (PlatformIO project).
- Build target: `env:cyd` (Arduino on ESP32).
- LVGL config: `lv_conf.h`.
- Build artifacts (after `pio run`): `firmware/src/display/CYD/.pio/build/cyd/firmware.bin`, plus `bootloader.bin` and `partitions.bin`.

## Hardware required
- Cheap Yellow Display / ESP32-2432S028 (240×320 ILI9341 + XPT2046 touch).
- USB-C or Micro-USB cable for flashing (depends on your board revision).
- USB-to-UART driver for your OS (CH9102/CP2102/CH340 as fitted to your board).

## Quick usage (post-flash)
- Connect the CYD to the same Wi‑Fi network as the controllers (or use Direct with the controller’s AP if needed).
- Put the controller into pairing mode (3 s hold on the controller’s pair button; LEDs alternate white/blue), then assign it to Fresh/Waste from the CYD setup buttons. The CYD remembers assignments.
- From the Fresh/Waste screens you can Fill, Drain, Clear Faults (now clears immediately on both CYD and controller), and Restart. Calibration sliders and overrides live under the Settings overlays.
- Faults, leak status, signal strength, and firmware versions are mirrored on the CYD in real time.

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
- Boot screen offering **Wi‑Fi** or **Direct** (Wi‑Fi is primary; Direct is for controller AP fallback).
- Home cards for **Fresh** and **Waste** tanks; once paired, they show live levels, temps, leak status, and connection health.
- Buttons for **Fill**, **Drain**, **Clear Faults**, and **Restart** per tank. Clear Faults immediately clears the local UI and sends a clear command to the controller.
- Settings screen with brightness, sleep timeout, theme toggle, and firmware label `v0.2.0`.

## Updating
- Rebuild and re-flash using either method above. Brightness, sleep timeout, theme selections, and stored controller assignments persist on the display between updates.

## Next steps (planned)
- Broader direct-mode support without Wi‑Fi infrastructure.
- Optional haptic/beeper cues and richer fault details on-screen.
- Ship signed/prebuilt binaries alongside source for easier end-user installs.
