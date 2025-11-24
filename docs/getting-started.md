# Getting Started – SmartRV TankPro v3.0

## Hardware required
- SmartRV TankPro v3.0 PCB/assembly (order files in [`hardware/fabrication`](../hardware/fabrication/)).
- 12–24 V DC supply from the RV/van/boat (with inline fuse per wiring guidance).
- One or two resistive tank senders:
  - Sensor 1: ~0–190 Ω range.
  - Sensor 2: ~33–240 Ω range.
- Relay load (pump/valve) if required.
- USB-C cable for flashing and debugging.

## Build/flash workflow
1. **Fabricate/assemble hardware:** Submit Gerbers/BOM in [`hardware/fabrication`](../hardware/fabrication/) to your PCB house (e.g., JLCPCB) or build from source in [`hardware/design`](../hardware/design/).
2. **Wire power and sensors:** Follow the connector pin-outs in [`docs/wiring-and-installation.md`](wiring-and-installation.md). Add an inline fuse on the 12–24 V feed.
3. **Flash firmware:**
   - Install ESPHome.
   - Flash the provided [`water-tank-module-v3.yaml`](../firmware/src/controller/water-tank-module-v3.yaml) to an ESP32-C3 module on the board over USB-C.
   - Set Wi‑Fi credentials via ESPHome or `secrets.yaml` before flashing.
4. **Connect and verify:**
   - Power the board from RV supply or USB-C.
   - Connect to the ESPHome dashboard or directly to the device IP to confirm sensor readings and relay control.

## Basic usage
- Use Home Assistant or ESPHome dashboard to view tank percentages and control the pump/valve relay.
- Status LEDs and optional buzzer provide local feedback (patterns defined in firmware).
- For calibration, record ADC readings at empty/full and adjust the calibration blocks in the ESPHome YAML (see [`firmware/src/controller/README.md`](../firmware/src/controller/README.md)).
