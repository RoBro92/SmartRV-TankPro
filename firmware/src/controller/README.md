# Firmware – Controller (ESP32-C3)

This directory contains ESPHome-based firmware for SmartRV TankPro v3.0. The goal is to expose tank level, relay control, LEDs, buzzer, leak input, and temperature to Home Assistant or the ESPHome dashboard.

## Files
- [`tankpro.yaml`](tankpro.yaml): Production ESPHome configuration (factory AP provisioning) for ESP32-C3 board.
- [`tankpro-esp32c6.yaml`](tankpro-esp32c6.yaml): Test build for Waveshare ESP32-C6-DevKit N8 using the same pin map (verify hardware availability).

## Building and flashing
1. Install [ESPHome](https://esphome.io/).
2. Connect the board over USB-C. ESPHome will use the USB CDC serial device exposed by the ESP32-C3.
3. Run `esphome run tankpro.yaml` to compile and flash.
4. On first boot the device exposes an AP `SmartRV-TankPro-Setup` (password `changeme`). Connect to the captive portal at `192.168.4.1` or use Improv Serial to provide Wi‑Fi credentials.

## Pin mapping
- Tank level sensor via divider: GPIO0 (ADC)
- Temperature sensor (DS18B20): GPIO1 (Dallas 1-Wire)
- Leak sensor input: GPIO3 (digital)
- Buzzer: GPIO4
- Relay/valve: GPIO5
- WS2812 LED 1: GPIO6
- WS2812 LED 2: GPIO7
- Pairing button: GPIO10 (momentary, triggers restart/provisioning)
- UART header for display/debug: TX=GPIO21, RX=GPIO20 (board-specific)

## Calibration guidance
1. With the tank empty, note the ADC reading for the tank level input in the ESPHome logs; set this as the 0% reference in the `calibrate_linear` block.
2. Fill the tank (or use a resistor that matches the “full” level for the sender) and note the ADC reading; set this as the 100% reference.
3. Save and re-flash to apply new calibration points. Additional points can be added for non-linear senders.

## Wi‑Fi and API
- Boots into a factory AP (`SmartRV-TankPro-Setup` / `changeme`) with captive portal and web UI; use this to provision Wi‑Fi.
- Native API is enabled with no reboot timeout to support long provisioning sessions.
- OTA is enabled; set an OTA password in `tankpro.yaml` before production.

## Home Assistant integration
After flashing, the device will appear in Home Assistant via ESPHome discovery. Entities include:
- Tank level percentage.
- DS18B20 tank temperature.
- Leak sensor binary input.
- Relay switch entity for pump/valve control.
- Buzzer control.
- Two WS2812 LED channels with effects.

## Licensing
SmartRV TankPro firmware is released under the MIT License; see `LICENSE-SOFTWARE` at the repo root. ESPHome itself remains under its own license; see the ESPHome project for details.
