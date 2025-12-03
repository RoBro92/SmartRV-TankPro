# Cheap Yellow Display – Firmware Install & Updates

Step-by-step guide for flashing the LVGL firmware onto a 2.8" Cheap Yellow Display (ESP32-2432S028). The current build label shown on the Settings screen is **v0.0.1**.

## What you need
- A Cheap Yellow Display (ESP32-2432S028) with USB cable.
- USB-to-UART driver for your OS (CH9102/CP2102/CH340 depending on your board).
- PlatformIO CLI (recommended) or VS Code + PlatformIO extension.
- This repo checked out locally.

## Flash from source with PlatformIO (recommended)
```bash
cd firmware/src/display/CYD
pio run -e cyd                                    # build
pio run -t upload -e cyd --upload-port <port>     # flash (e.g., /dev/cu.usbserial-XXXXX or COM5)
pio device monitor -b 115200 --port <port>        # optional: watch boot/log output
```
- If the board does not auto-enter bootloader: hold `BOOT`, tap `RST`, then release `BOOT` once upload starts.
- Power via USB during flashing; 5 V/GND header also works for bench power.

## Flash prebuilt binaries with esptool.py
1) Build once with PlatformIO so the binaries exist, or obtain the three files from `firmware/src/display/CYD/.pio/build/cyd/` (`bootloader.bin`, `partitions.bin`, `firmware.bin`).  
2) Run:
```bash
cd firmware/src/display/CYD/.pio/build/cyd
esptool.py --chip esp32 --baud 460800 --port <port> --before default_reset --after no_reset write_flash -z \
  0x1000  bootloader.bin \
  0x8000  partitions.bin \
  0x10000 firmware.bin
```

## After flashing
- On boot you should see the TankPro UI; log output is available at 115200 baud (`pio device monitor -b 115200 --port <port>`).
- Display settings (brightness, sleep timeout, theme) persist across reboots on the display itself.
- If icons render with odd colors, confirm you flashed the matching `firmware.bin` from this build and that the panel is the ESP32-2432S028 variant (ILI9341 + XPT2046).

## Updating later
- Repeat the same flashing steps with the new build. No special migration is required; settings are kept on the display.
- If you prefer not to install PlatformIO, you can flash a shared `firmware.bin` with the esptool.py method above.
