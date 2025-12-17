# SmartRV TankPro (controller) – package layout

This ESPHome config is split into packages to keep it maintainable. No behaviour changes are implied by the split; order matters.

Package order in `tankpro.yaml`:
1. `packages/00_substitutions.yaml` – substitutions/constants
2. `packages/01_base.yaml` – esphome/esp32/logger/ota/wifi/api/time/etc
3. `packages/10_state_globals.yaml` – globals and stored state/config
4. `packages/20_hardware.yaml` – hardware bindings (lights/outputs/switches/buttons)
5. `packages/30_sensors.yaml` – sensors and related intervals
6. `packages/40_faults.yaml` – fault evaluation, safety guards, annunciation
7. `packages/50_comms_wifi.yaml` – Wi‑Fi/UDP comms (pairing adverts, config/cmd RX/TX)
8. `packages/51_comms_direct.yaml` – Direct ESP‑NOW transport (pairing/state/commands)
9. `packages/60_services_scripts.yaml` – general scripts/services glue (non‑fault, non‑comms)

Helper header:
- `includes/tankpro_helpers.h` – shared helpers for MAC utils, config packing, fault set/clear.

Quick validation/build:
```
cd firmware/src/controller/esphome
esphome compile tankpro.yaml     # full build
# or, for a quick structure check:
esphome config tankpro.yaml
```
