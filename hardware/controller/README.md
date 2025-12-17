# Hardware – Controller (v0.2.0 release, hardware rev v3)

The TankPro controller is an ESP32-C3-based tank monitor/valve driver for RVs/boats. It supports standard resistive tank senders, drives a valve relay and buzzer/LEDs, and pairs with the Cheap Yellow Display (CYD) primary node.

## Key features
- ESP32-C3 with USB-C for power/programming and automotive 12–24 V input path.
- Dual resistive level inputs tuned for ~0–190 Ω and ~33–240 Ω senders.
- Relay driver for valve control, onboard buzzer, and dual WS2812 status LEDs.
- Pairing with the CYD display (Wi-Fi or Direct ESP-NOW); supports two roles: Fresh and Waste.
- Factory-reset button with long-press behaviour; direct-mode lock persists until reset.

## Parts and install (overview)
- Required: ESP32-C3 TankPro PCB (rev v3 outputs in `hardware/fabrication/`), valve/relay, tank sender wiring, 12–24 V supply.
- Mount the PCB in a dry location, wire the sender to the ADC input, wire the valve relay output, and connect 12–24 V with the onboard fuse.
- USB-C may be used for bench power/programming; automotive input is preferred for normal use.

## Safety notes
- Protect the supply with the onboard fuse; observe polarity.
- Valve relay is low-side switched; ensure load current is within the relay specs.
- Use shielded wiring for sensors where possible; keep sensor leads away from high-current paths.

## Pairing with the display
- Power the controller; if not provisioned and pairing is enabled, it advertises for the CYD.
- The CYD assigns a role (Fresh/Waste) and saves the peer. Direct-mode lock prevents future Wi-Fi provisioning until factory reset.

## Version
- Hardware reference: rev v3 (unchanged in this release).
- Release documentation: v0.2.0.
