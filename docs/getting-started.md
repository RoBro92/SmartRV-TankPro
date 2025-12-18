# Getting started

Single-page setup guide for TankPro v0.2.0: controller + CYD + Home Assistant.

## Modes at a glance
- Wi‑Fi mode: controller on your LAN, visible to HA via ESPHome.
- Direct mode (ESP‑NOW): controller ↔ CYD without an AP; auto-selected when pairing and no Wi‑Fi creds.
- Standalone with Home Assistant: run controller alone in Wi‑Fi mode (no CYD) and adopt via ESPHome.

## Error codes (controller)
- 1: Leak detected → resolve leak, Clear Faults.
- 2: Valve open without active fill → valve forced off; Clear Faults and restart fill if safe.
- 3: Freeze protection active (temp below threshold) → valve off; warms up clears it.
- 4: Temp sensor failure/timeout → check DS18B20 wiring/sensor; Clear Faults after fix.
- 9: Lost connection to CYD (Direct) → restore link or factory reset/re-pair.

## LED quick meanings (controller)
- Network LED: red (isolated), white flash (captive portal), blue solid (connected), yellow flash (link lost), purple (OTA).
- Status LED: green (idle), yellow solid (valve on), orange flash (warning), red fast flash (fault), alternating red for reset/fault alert.

## Install: controller firmware
- Prebuilt (recommended for most users): flash `releases/v0.2.0/controller-v0.2.0.bin` via ESPHome (`esphome run ... --device <port>`) or esptool.
- Build yourself: `cd firmware/src/controller/esphome && esphome run tankpro.yaml --device <port>`
- On first boot with no creds + pairing off: captive portal AP `TankPro-Setup` (`changeme`).
- Standalone HA: keep in Wi‑Fi mode and add via ESPHome integration in Home Assistant.
- Minimal HA-only config: `firmware/src/controller/esphome/tankpro_basic.yaml` (no CYD logic).

## Install: CYD firmware (display)
- Prebuilt: use release binaries in `releases/v0.2.0/` (`cyd-bootloader-0.2.0.bin`, `cyd-partitions-0.2.0.bin`, `cyd-firmware-0.2.0.bin`) with esptool or PlatformIO upload.
- Build/flash from `firmware/src/display/CYD`: `pio run -e cyd_s3` (or `pio run -t upload -e cyd_s3 --upload-port <port>`).
- Binaries: `.pio/build/cyd_s3/` (bootloader, partitions, firmware) if you build locally.

## Pairing with CYD (Direct or Wi‑Fi)
1) Controller: enable pairing (button hold per LED prompt) or power up with no creds + pairing on.
2) CYD: Assign Roles overlay → pick controller advert → assign Fresh/Waste.
3) CYD sends role/key; controller locks Direct mode (until factory reset). Rejoins automatically after reboot.

## Factory reset + lock
- Long-press pairing button to factory reset; clears Wi‑Fi creds and Direct peer/key so provisioning can run again.

## Troubleshooting basics
- No telemetry: confirm LEDs, check mode; factory reset and re-pair if needed.
- Valve stuck: Clear Faults; verify thresholds; power cycle if relay latched.
- Portal not showing: Direct lock active; factory reset.
- OTA fails: power via USB-C, retry `esphome run` or ESPHome Dashboard.
