# Firmware – Controller (v0.2.0)

ESPHome-based firmware for the TankPro controller (ESP32-C3). Runs in two user-facing modes:
- **Wi‑Fi mode:** normal home network operation (provisioned via captive portal/Improv).
- **Direct mode (ESP‑NOW):** controller ↔ CYD link without an access point (auto-selected when pairing and no Wi‑Fi creds; locked until factory reset).

## What it does
- Reads tank level voltage and temperature, detects leak input, and reports faults.
- Drives the valve relay/buzzer/LEDs; executes Fill/Drain/Restart/Clear Faults commands from CYD.
- Publishes level/temp/fault/relay state continuously to CYD over Wi‑Fi or Direct.

## Pairing flow (with CYD display)
1) Power controller. If unprovisioned + pairing enabled, it advertises for the CYD (Direct) instead of starting Wi‑Fi portal.
2) On CYD, open Assign Roles overlay, select the controller, assign Fresh or Waste.
3) CYD sends role + key; controller locks to Direct mode and stores peer MAC/key.
4) After pairing, controller rejoins automatically after reboot and resumes telemetry.

## Factory reset + direct-mode lock
- Long-press pairing button (per on-screen prompt) to factory reset. This clears Wi‑Fi creds and Direct lock/keys.
- While Direct lock is present, controller ignores new Wi‑Fi creds and never enters provisioning.

## Basic troubleshooting
- No telemetry in CYD: check LEDs, ensure Direct lock peer MAC matches; factory reset and re-pair if needed.
- Valve won’t stop: Clear Faults; verify fill/drain states and thresholds in CYD config.
- Wi‑Fi portal not appearing: Direct lock likely active—factory reset to return to Wi‑Fi provisioning.
- Leak/fault latched: resolve sensor condition, then issue Clear Faults from CYD.
- OTA issues: ensure USB-C power is stable; retry `esphome run` or OTA from ESPHome Dashboard.

## Build/flash
- Install ESPHome, connect over USB-C, then run from this folder:
  - `esphome run esphome/tankpro.yaml`
- Version reported via `firmware_version` substitution: **v0.2.0**.

Docs/index/roadmap: see `docs/README.md`.
Images/assets: see `docs/assets/` (icons/diagrams if added later).

## Changelog
See `CHANGELOG.md` in this directory.
