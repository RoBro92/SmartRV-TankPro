# Getting Started – SmartRV TankPro v3.0

## Hardware required
- SmartRV TankPro v3.0 PCB/assembly (order files in [`hardware/fabrication`](../hardware/fabrication/)).
- 12–24 V DC supply from the RV/van/boat.
- One of two resistive tank senders:
  - Sensor 1: ~0–190 Ω range.
  - Sensor 2: ~33–240 Ω range.
- Relay load (pump/valve) if required. Should be a 2 wire self closing OR 3 wire valve.
- USB-C cable for flashing and debugging. (not required if you purchase from me as will come pre programmed)

## Build/flash workflow
1. **Fabricate/assemble hardware:** Submit Gerbers/BOM in [`hardware/fabrication`](../hardware/fabrication/) to your PCB house (e.g., JLCPCB) or build from source in [`hardware/design`](../hardware/design/).
2. **Wire power and sensors:** Follow the connector pin-outs in [`docs/wiring-and-installation.md`](wiring-and-installation.md).
3. **Flash firmware:**
   - Install ESPHome.
   - Flash the provided [`tankpro.yaml`](../firmware/src/controller/esphome/tankpro.yaml) to an ESP32-C3 module on the board over USB-C using ESPHome.
   - On first boot, connect to the factory AP `SmartRV-TankPro-Setup` (password `changeme`) and use the captive portal at `192.168.4.1` to set Wi‑Fi.
4. **Connect and verify:**
   - Power the board from RV supply or USB-C.
   - Connect to the ESPHome dashboard or directly to the device IP to confirm sensor readings and relay control.

## Basic usage
- Use Home Assistant or ESPHome dashboard to view tank percentages and control the pump/valve relay.
- Status LEDs and optional buzzer provide local feedback (patterns defined in firmware).
- For calibration, record ADC readings at empty/full and adjust the calibration blocks in the ESPHome YAML (see [`firmware/src/controller/README.md`](../firmware/src/controller/README.md)).

## Controller indicators (LEDs & buzzer)
- **LED1 – Network (top LED):** Off until boot completes. Red solid = no Wi‑Fi/CYD link. White slow flash = captive portal active. Blue solid = connected. Yellow slow flash = connection lost/retrying. Blue fast flash = connecting. Purple slow flash = OTA update. Pairing mode alternates white/blue, and any connectivity change briefly double-flashes yellow.
- **LED2 – System/relay (bottom LED):** Warm white pulse = boot; green solid = normal/relay off; yellow solid = relay on; orange slow flash = warning/non-critical fault; red fast flash = critical fault or safety trip; purple fast flash = OTA. Faults and factory-reset warning show alternating red flashes across both LEDs.
- **Buzzer:** Only used for fault chirps and optional safety cues; muted during OTA.

## Pair button behaviors
- **Short press (<1 s):** Toggle relay when not in fault/reset/pairing. If pairing mode is active, a short press cancels pairing without toggling the relay.
- **Enter pairing:** Hold ~3 s. LEDs immediately alternate white/blue while held. Pairing stays active until you press again or a factory reset is triggered.
- **Factory reset warning:** Hold 10–15 s. Both LEDs alternate red during the warning and pairing mode is disabled. Releasing early cancels. Holding to 15 s performs factory reset and clears pairing.
- **Clear faults:** Use CYD/HA “Clear Faults” or short press the relay toggle after the fault condition is resolved; LEDs return to their normal state automatically.
