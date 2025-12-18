# SmartRV TankPro (v0.2.0)

Open-source tank monitoring/control system for RVs/boats: ESP32-C3 controllers on each tank plus a Cheap Yellow Display (CYD) as the primary node.


## Key features
- Works over Wi‑Fi or Direct (ESP‑NOW) between CYD and controllers.
- Fresh + Waste roles, max two controllers per display.
- Live level/temperature/leak/fault telemetry and relay control (fill/drain).
- On-device safety (fault detection, failsafe valve shutoff on comms loss).
- Factory reset + direct-mode lock for secure pairing.
- ESPHome-based controller firmware; LVGL-based CYD firmware.
- Controller can also run standalone with Home Assistant (ESPHome integration).

## How it works
- Controller reads tank sensors, drives relay/buzzer/LEDs, and reports state.
- CYD discovers controllers, assigns roles, shows status, and sends commands.
- Link can be Wi‑Fi (UDP) on your LAN or Direct ESP‑NOW (channel 6) when no AP.

## Repository map
- `hardware/controller/` – hardware overview + changelog (fabrication files in `hardware/fabrication/`).
- `firmware/src/controller/` – ESPHome controller firmware (README + changelog).
- `firmware/src/display/` – CYD firmware (PlatformIO LVGL) with README + changelog.
- `docs/` – reference guides/assets (see `docs/README.md`).
- `hardware/media/images/` – product photo, PCB renders, and CYD UI snapshots.
- `CHANGELOG.md` – project-wide changes.

## Getting started
- Prebuilt binaries (v0.2.0): controller `releases/v0.2.0/controller-v0.2.0.bin`, CYD `releases/v0.2.0/cyd-*-0.2.0.bin` (bootloader/partitions/firmware).
- You can also build from source if you prefer (commands below).
- Hardware: see `hardware/controller/README.md` (parts, wiring, safety).
- Controller firmware: `firmware/src/controller/README.md` (modes, pairing, flashing).
- CYD firmware: `firmware/src/display/README.md` (UI actions, flashing).
- Docs index: `docs/README.md`
- Getting started guide: `docs/getting-started.md`
- Roadmap: `docs/roadmap.md`
- Beta testing: `docs/beta-testing.md`

## Current limitations
- Direct mode (ESP‑NOW) may not recover cleanly after a controller power-cycle; prefer Wi‑Fi pairing where possible. If Direct link drops, factory reset the controller and re-pair.
- 0.2.0 focuses on core functionality; UI polish and Direct-link robustness are on the roadmap.

## Troubleshooting (quick)
- Portal not showing: Direct lock active; factory reset (long-press pairing) to enable Wi‑Fi provisioning.
- No telemetry: verify LEDs, ensure roles are assigned; re-pair or switch to Wi‑Fi mode.
- Relay stuck: issue Clear Faults, then Fill/Drain again; power-cycle if still latched.
- Direct link drops: re-pair or temporarily use Wi‑Fi; note the limitation above.
- Need help? Open a GitHub Issue (labels: `type:bug`, `type:feature`, `area:controller-fw`, `area:display-fw`).

## Demo videos
- (placeholder links for quick demos)


## Licensing
- Hardware: CERN OHL-W v2 (`LICENSE-HARDWARE`).
- Software/Firmware: MIT (`LICENSE`).
