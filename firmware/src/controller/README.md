# Firmware – Controller (ESPHome)

This directory contains ESPHome-based firmware for SmartRV TankPro v3.0 (ESP32-C3 and ESP32-S3 variants).

## Files
- `esphome/tankpro.yaml`: Production ESPHome configuration for the ESP32-C3 board (factory AP provisioning).
- `esphome/tankpros3.yaml`: Production ESPHome configuration for the ESP32-S3 board with on-device automations and safety logic.
- `esphome/tankpro_basic.yaml`: Barebones ESP32-S3 build that exposes only core I/O (no automations/safety) for users who prefer to create their own Home Assistant automations.

## Flashing (home use)
1) Install [ESPHome](https://esphome.io/).  
2) Connect the controller over USB-C.  
3) From this directory run `esphome run esphome/<file>.yaml` (choose the variant you want).  
4) On first boot the device exposes an AP `SmartRV-TankPro-Setup` (password `changeme`). Connect to the captive portal at `192.168.4.1` or use Improv Serial to provide Wi‑Fi credentials.  
5) The device will appear in Home Assistant via ESPHome discovery.

## TankPro ESP32-S3 (tankpros3.yaml)
- **Hardware pins**: Level ADC GPIO1; DS18B20 temp GPIO47; Leak GPIO8; Relay GPIO5; Buzzer GPIO4; Status WS2812 LED GPIO48; Button GPIO16.
- **Core features**: Fill/drain automations with stop thresholds, leak fault handling, freeze protection, OTA, web server, and persistent level calibration.
- **Status/diagnostics**: Fault code/description, system status, Wi‑Fi signal, uptime, freeze protection state.
- **Controls & config in Home Assistant**:
  - Buttons: Fill Tank, Drain Tank, Clear Faults, Restart Device.
  - Calibration buttons (Configuration section): Set Level Empty, Set Level Full.
  - Thresholds (Configuration): Fill Stop Level (%), Drain Stop Level (%), Freeze Protection Threshold (0.1–5.0 °C).
  - Switches (Configuration): Freeze Protection, Safety Override, Valve Override, Tank Buzzer.
  - Reset: **Reset Configuration** (Configuration) restores defaults, clears calibrations, freezes settings, and clears latched faults/state.
- **Calibration**:
  - Empty: With tank empty, press **Set Level Empty** to store the current voltage.
  - Full: With tank full, press **Set Level Full** to store the full voltage.
  - These points are saved to flash and used to compute percent.
- **Fault indication**: Status LED flashes red and buzzer pulses on fault; clears when the fault condition is resolved and you press **Clear Faults** or **Reset Configuration** (also turns off relay/buzzer/LED).

## TankPro Basic ESP32-S3 (tankpro_basic.yaml)
- Core I/O only: Button, leak sensor, valve relay, buzzer, WS2812 status LED, tank level voltage, temperature, Wi‑Fi signal, uptime, device info.
- No automations, safety, or on-device logic; intended for DIY automations in Home Assistant.

## TankPro ESP32-C3 (tankpro.yaml)
- Original C3 build with basic entities (level %, temp, leak, relay, buzzer, LEDs, restart button) and factory AP provisioning.

## Licensing
SmartRV TankPro firmware is released under the MIT License; see `LICENSE-SOFTWARE` at the repo root. ESPHome itself remains under its own license; see the ESPHome project for details.
