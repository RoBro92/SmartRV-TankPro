# Firmware

SmartRV TankPro firmware and configuration live here. All firmware/software content in this repository is released under the MIT License; see `LICENSE-SOFTWARE` in the repo root.

- `src/controller/`: ESPHome-based firmware for the controller (ESP32-C3) board.
  - Primary config: `tankpro.yaml` (factory AP provisioning, ESP32-C3).
  - Alt test build: `tankpro-esp32c6.yaml` (Waveshare ESP32-C6-DevKit N8; verify pin availability).
- `src/display/`: LVGL display client for the Cheap Yellow Display (ESP32-2432S028).
  - PlatformIO project lives in `src/display/CYD` (target `env:cyd`).
  - Built artifacts (after `pio run`) are under `src/display/CYD/.pio/build/cyd/` (`firmware.bin`, `bootloader.bin`, `partitions.bin`).
  - User flashing guide: `docs/display-firmware.md`.

Example or environment-specific configuration files should live alongside the relevant sources (e.g., additional ESPHome YAMLs) or under a future `config/` folder if needed.
