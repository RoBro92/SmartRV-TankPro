# SmartRV TankPro (v3.0)
[![Hardware Licence: CERN OHL-W v2](https://img.shields.io/badge/Hardware%20Licence-CERN%20OHL--W%20v2-blue)](#licensing) [![Software Licence: MIT](https://img.shields.io/badge/Software%20Licence-MIT-green)](#licensing)

SmartRV TankPro is an open-source, ESP32-C3-based tank monitoring module for caravans, RVs, and boats. Designed for 12–24 V DC systems, it supports industry-standard resistive level senders (0–190 Ω and 33–240 Ω), offers Wi‑Fi connectivity via ESPHome, and can be paired with an optional companion display.

**Status:** v3.0 hardware is the current recommended design. v1/v2 hardware is archived under `hardware/legacy/v1-v2/` for reference only.

## Repository structure
- `hardware/`: Hardware design assets (design sources, fabrication outputs, mechanical models) plus legacy archives.
- `firmware/`: ESP32-C3 firmware/configuration sources and the LVGL display client for the Cheap Yellow Display (ESP32-2432S028).
- `docs/`: User guides, wiring, design rationale, and changelog.

## Key features (v3.0)
- ESP32-C3 module with integrated USB programming.
- Dual supply input: 12–24 V DC primary plus USB-C for programming/bench.
- Flexible power path: fused RV input, buck to 5 V, 3.3 V LDO, and diode/ideal-diode mux between USB and DC.
- Two resistive tank channels (≈0–190 Ω and ≈33–240 Ω) with tailored dividers, series resistors, ADC decoupling, and ESD/transient protection.
- 5 V relay driver, WS2812C addressable LEDs, and optional buzzer.
- UART/USB for debugging and future external display integration.

## How to build / fabricate
- **Hardware:** Use the outputs in `hardware/fabrication/` to order PCBs/assembly from your board house (e.g., JLCPCB). Mechanical models live in `hardware/mechanical/`. Editable schematic/PCB sources will be published in `hardware/design/` in a future OSHW release.
- **Firmware (ESPHome controller):** Install ESPHome, connect the ESP32-C3 over USB-C, and run `esphome run firmware/src/controller/tankpro.yaml` to compile and flash. On first boot the device exposes `SmartRV-TankPro-Setup` (password `changeme`) for captive-portal provisioning; then adopt in ESPHome/Home Assistant. Wiring guidance is in `docs/wiring-and-installation.md`.
- **Display firmware (Cheap Yellow Display):** LVGL demo lives in `firmware/src/display/CYD` (`env:cyd`). Build/flash with PlatformIO or `esptool.py` using the binaries under `.pio/build/cyd/`. User-friendly install/update steps for a store-bought Cheap Yellow Display are in `docs/display-firmware-installation.md`; technical notes remain in `docs/display-firmware.md`.

## Documentation
- `docs/overview-v3.md`
- `docs/getting-started.md`
- `docs/wiring-and-installation.md`
- `docs/display-firmware-installation.md`
- `docs/display-firmware.md`
- `docs/design-rationale-v3.md`
- `docs/changelog.md`

## Licensing
- **Hardware:** Licensed under **CERN OHL-W v2**. See `LICENSE-HARDWARE`. Hardware files live primarily in `hardware/`. At present, only fabrication outputs (Gerbers, etc.) are included. Editable design source files (schematics and PCB layouts) will be added in a future open source hardware (OSHW) release.
- **Software/Firmware:** Licensed under the **MIT License**. See `LICENSE`. Firmware lives in `firmware/`.

## How to support the project
- Build it yourself with the open-source files provided.
- Prefer to buy? Support development by purchasing assembled boards or kits from the project website (link to be added).
- Donations or contributions (issues/PRs) are welcome and help fund ongoing work.

## Contributing
Pull requests and issues are welcome. Please open an issue to discuss significant changes or new features.
