# Controller Firmware Changelog

## v0.2.0
- Wi‑Fi and Direct (ESP‑NOW) comms running with live telemetry (level/temp/fault/relay) and config commands.
- Pairing/role assignment via CYD; direct-mode lock persists until factory reset.
- Failsafe closes valve if comms lost while active; fault set/clear streamlined.
- Firmware packaging reorganised into packages + helpers (no user-visible behaviour change).

## v0.1.0
- Production ESP32-C3 profile with refined LEDs, pairing UX, relay/pair-button safeguards, reset/fault signalling.
- CYD commands supported: Fill, Drain, Restart, Clear Faults; live state reporting.
