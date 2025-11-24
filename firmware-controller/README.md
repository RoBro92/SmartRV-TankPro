# Firmware – Controller (ESP32-C3)

This directory contains ESPHome-based firmware for SmartRV TankPro v3.0. The goal is to expose tank levels, relay control, LEDs, and optional buzzer to Home Assistant or the ESPHome dashboard.

## Files
- [`water-tank-module-v3.yaml`](water-tank-module-v3.yaml): Primary ESPHome configuration.

## Building and flashing
1. Install [ESPHome](https://esphome.io/).
2. Copy `water-tank-module-v3.yaml` and add your Wi‑Fi credentials (either inline under `wifi:` or via a shared `secrets.yaml`).
3. Connect the board over USB-C. ESPHome will use the USB CDC serial device exposed by the ESP32-C3.
4. Run `esphome run water-tank-module-v3.yaml` to compile and flash.

## Pin mapping
- Sensor 1 (0–190 Ω): GPIO0 (ADC)
- Sensor 2 (33–240 Ω): GPIO1 (ADC)
- Relay control: GPIO10
- WS2812C LEDs: GPIO8
- Buzzer (optional): GPIO9
- UART header for display/debug: TX=GPIO21, RX=GPIO20

## Calibration guidance
1. With the tank empty, note the ADC reading for each sensor in the ESPHome logs; set this as the 0% reference in the `calibrate_linear` blocks.
2. Fill the tank (or use a resistor that matches the “full” level for the sender) and note the ADC reading; set this as the 100% reference.
3. Save and re-flash to apply new calibration points. Additional points can be added for non-linear senders.

## Wi‑Fi and API
- The YAML exposes the ESPHome native API and an optional captive portal for first-time setup.
- Set a device name and passwords in the YAML to secure OTA and API access.

## Home Assistant integration
After flashing, the device will appear in Home Assistant via ESPHome discovery. Entities include:
- Tank level sensor for each channel (percentage).
- Relay switch entity for pump/valve control.
- Status LEDs and optional buzzer controls (exposed as lights/switches depending on configuration).
