# Firmware

SmartRV TankPro firmware and configuration live here. All firmware/software content in this repository is released under the MIT License; see `LICENSE-SOFTWARE` in the repo root.

- `src/controller/`: ESPHome-based firmware for the controller (ESP32-C3) board.
  - Primary config: `tankpro.yaml` (factory AP provisioning, ESP32-C3).
  - Alt test build: `tankpro-esp32c6.yaml` (Waveshare ESP32-C6-DevKit N8; verify pin availability).
- `src/display/`: Placeholder for a future external display client.

Example or environment-specific configuration files should live alongside the relevant sources (e.g., additional ESPHome YAMLs) or under a future `config/` folder if needed.
