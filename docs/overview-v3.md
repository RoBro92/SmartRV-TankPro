# SmartRV TankPro v3.0 – Overview

SmartRV TankPro v3.0 is an ESP32-C3-based controller for monitoring and controlling fresh/grey/black water tanks in caravans, RVs, and boats. It is designed for 12–24 V automotive-style systems and supports industry-standard resistive tank level sensors.

## System at a glance
- **Inputs:**
  - 12–24 V DC supply (primary) and USB-C (secondary/programming).
  - Support for two resistive level senders: Sensor 1 (~0–190 Ω) and Sensor 2 (~33–240 Ω).
  - Leak Sensor
  - Temperature sensor via DS18B20
- **MCU:** ESP32-C3 with USB programming and Wi‑Fi connectivity.
- **Outputs / interfaces:**
  - 5 V relay output for pump/valve control.
  - WS2812C addressable status/level LEDs.
  - Passive buzzer for alarms/feedback.
  - UART/USB for debugging.

## Intended use
- Provide reliable tank level monitoring with tailored analog front-ends per sender range for better resolution.
- Integrate with Wi‑Fi dashboards (e.g., ESPHome/Home Assistant) and an external display client (Cheap Yellow Display LVGL demo in-progress).
- Fit common RV/caravan/boat wiring practices, including protection against reverse polarity, ESD, and wiring mistakes.

## Hardware folder map
- [`hardware/design`](../hardware/design/): EDA source files (EasyEDA Pro preferred; export KiCad if available).
- [`hardware/fabrication`](../hardware/fabrication/): Gerbers, drill files, pick-and-place, BOM for fabrication/assembly houses.
- [`hardware/mechanical`](../hardware/mechanical/): Enclosure and mechanical models.

## Firmware map
- [`firmware/src/controller`](../firmware/src/controller/): ESPHome configuration for the controller PCB.
- [`firmware/src/display`](../firmware/src/display/): LVGL demo firmware for the Cheap Yellow Display (ESP32-2432S028); UI is present, controller integration is still in-progress.
