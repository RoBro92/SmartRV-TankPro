# SmartRV TankPro v3.0 – Overview

SmartRV TankPro v3.0 is an ESP32-C3-based controller for monitoring and controlling fresh/grey/black water tanks in caravans, RVs, and boats. It is designed for 12–24 V automotive-style systems and supports industry-standard resistive tank level sensors.

## System at a glance
- **Inputs:**
  - 12–24 V DC supply (primary) and USB-C (secondary/programming).
  - Two resistive level senders: Sensor 1 (~0–190 Ω) and Sensor 2 (~33–240 Ω).
- **MCU:** ESP32-C3 with USB programming and Wi‑Fi connectivity.
- **Outputs / interfaces:**
  - 5 V relay output for pump/valve control.
  - WS2812C addressable status/level LEDs.
  - Optional passive buzzer for alarms/feedback.
  - UART/USB for debugging and future external display integration.

## Intended use
- Provide reliable tank level monitoring with tailored analog front-ends per sender range for better resolution.
- Integrate with Wi‑Fi dashboards (e.g., ESPHome/Home Assistant) and future remote display hardware.
- Fit common RV/caravan/boat wiring practices, including protection against reverse polarity, ESD, and wiring mistakes.

## Hardware folder map
- [`hardware-v3/pcb`](../hardware-v3/pcb/): EDA source files (EasyEDA Pro preferred; export KiCad if available).
- [`hardware-v3/manufacturing`](../hardware-v3/manufacturing/): Gerbers, drill files, pick-and-place, BOM for fabrication/assembly houses.
- [`hardware-v3/3d`](../hardware-v3/3d/): Enclosure and mechanical models.

## Firmware map
- [`firmware-controller`](../firmware-controller/): ESPHome configuration for the controller PCB.
- [`firmware-display`](../firmware-display/): Reserved for a future external display unit that will consume data from the controller over Wi‑Fi or serial.
