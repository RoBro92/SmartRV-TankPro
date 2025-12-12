# Firmware – Controller (ESPHome)

This directory contains ESPHome-based firmware for SmartRV TankPro v3.0.

## Files
- `esphome/tankpro.yaml`: Production ESPHome configuration for the ESP32-C3 board (factory AP provisioning, CYD pairing, on-device safety).
- `esphome/tankpro_basic.yaml`: Barebones ESP32-S3 build that exposes only core I/O (no automations/safety) for users who prefer to create their own Home Assistant automations.

## Flashing (home use)
1) Install [ESPHome](https://esphome.io/).  
2) Connect the controller over USB-C.  
3) From this directory run `esphome run esphome/tankpro.yaml` (or `tankpro_basic.yaml` if you intentionally want the S3 barebones build).  
4) On first boot the device exposes an AP `SmartRV-TankPro-Setup` (password `changeme`). Connect to the captive portal at `192.168.4.1` or use Improv Serial to provide Wi‑Fi credentials.  
5) The device will appear in Home Assistant via ESPHome discovery.

## TankPro ESP32-C3 (tankpro.yaml)
- **Hardware pins**: Relay GPIO5, buzzer GPIO4, LEDs on GPIO6/7, pairing button GPIO10, leak GPIO3, tank level ADC GPIO0, tank temp DS18B20 GPIO1.
- **States (LED2)**: boot warm-white pulse, healthy green (relay off), yellow solid when relay energised, orange slow flash on warnings, red fast flash on critical faults, purple fast flash during OTA, alternating red flash during fault or reset warning.
- **Network (LED1)**: Off until boot completes; red solid when isolated, white slow flash for captive portal, blue solid when connected, yellow slow flash if link lost, blue fast flash while connecting, purple slow flash during OTA; brief double-yellow flash on connectivity state changes.
- **Pair button**: short press toggles relay (only when not pairing/fault/reset), 3 s hold enters pairing (LED1/LED2 alternate blue/white), pressing again cancels pairing without toggling relay, 10–15 s hold shows red alternating reset warning, 15 s hold factory resets and exits pairing, release returns to normal LED state.
- **Fault handling**: Both LEDs alternate red on critical faults; clear via CYD/HA or resolving condition plus clear-fault command.

## TankPro Basic ESP32-S3 (tankpro_basic.yaml)
- Core I/O only: button, leak sensor, valve relay, buzzer, WS2812 status LED, tank level voltage, temperature, Wi‑Fi signal, uptime, device info.
- No automations or on-device safety; intended for DIY automations in Home Assistant.

## Licensing
SmartRV TankPro firmware is released under the MIT License; see `LICENSE-SOFTWARE` at the repo root. ESPHome itself remains under its own license; see the ESPHome project for details.
