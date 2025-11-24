# SmartRV Water Tank Module (v3.0)

Open-source, ESP32-C3-based water tank monitoring module for caravans, RVs, and boats. Designed for 12–24 V DC systems, it supports industry-standard resistive level senders (0–190 Ω and 33–240 Ω), offers Wi‑Fi connectivity, and can be paired with an optional companion display.

## Project status
- **v3.0 is the current recommended hardware.**
- **v2.x hardware is now legacy** and retained only for reference in [`/hardware-v2-legacy`](hardware-v2-legacy/).
- The **`main` branch is the canonical source** for v3.0 hardware, firmware, and documentation updates.

## Feature summary (v3.0)
- **ESP32-C3 module** with integrated USB programming.
- **Dual supply input:**
  - Primary: 12–24 V DC RV supply.
  - Secondary: USB-C for programming and bench testing.
- **Power path options:**
  - Input fuse and polarity protection on RV input.
  - Buck regulator to 5 V rail and 3.3 V LDO for ESP32 and logic.
  - Either an ideal-diode power mux IC (e.g., LM66200) or dual Schottky OR-ing (SS34/SS54) between USB and DC.
- **Sensor inputs:**
  - Two resistive tank channels: Sensor 1 (≈0–190 Ω) and Sensor 2 (≈33–240 Ω).
  - Dedicated resistor dividers per range, series input resistors (~1 kΩ), local 100 nF ADC decoupling, and ESD/transient protection at the connector/ADC node.
- **Outputs / user interface:**
  - 5 V relay output for pump/valve control with transistor driver and flyback diode.
  - WS2812C addressable LEDs for status/level display and debug.
  - Optional passive buzzer driven via transistor with optional pull-down on the ESP32 pin.
- **Debug and programming:**
  - Exposed USB-C for flashing and serial monitor.
  - Access to RX/TX and D+/D− as required by ESP32-C3 programming.
  - Silkscreened labels and optional PCB QR code linking back to this repo.

## Quick start
1. Order PCBs/assembled boards using files in [`/hardware-v3/manufacturing`](hardware-v3/manufacturing/).
2. Wire tank sensors and RV power following the pin-outs in [`docs/wiring-and-installation.md`](docs/wiring-and-installation.md).
3. Flash the controller firmware from [`/firmware-controller`](firmware-controller/) (ESPHome YAML provided) and set Wi‑Fi credentials.
4. Power up, connect to Wi‑Fi, and verify tank level readings.

## Repository structure
- [`hardware-v3/`](hardware-v3/): Primary v3.0 hardware design files (EDA sources in `pcb/`, fabrication data in `manufacturing/`, and mechanical models in `3d/`).
- [`hardware-v2-legacy/`](hardware-v2-legacy/): Archived v1/v2 hardware, including the original README and design folders.
- [`firmware-controller/`](firmware-controller/): ESP32-C3 firmware for the tank module (ESPHome configuration provided).
- [`firmware-display/`](firmware-display/): Placeholder for future external display firmware.
- [`docs/`](docs/): v3.0 documentation set (overview, getting started, wiring/install, design rationale, changelog).

## Documentation
- [`docs/overview-v3.md`](docs/overview-v3.md)
- [`docs/getting-started.md`](docs/getting-started.md)
- [`docs/wiring-and-installation.md`](docs/wiring-and-installation.md)
- [`docs/design-rationale-v3.md`](docs/design-rationale-v3.md)
- [`docs/changelog.md`](docs/changelog.md)

## Firmware
- [`/firmware-controller`](firmware-controller/): ESP32-C3 firmware for the water tank module (ESPHome configuration provided).
- [`/firmware-display`](firmware-display/): Placeholder for future external display firmware.

## Contributing
Pull requests and issues are welcome. Please open an issue to discuss significant changes or new features.

## License
This project is released under the terms of the included [LICENSE](LICENSE).
