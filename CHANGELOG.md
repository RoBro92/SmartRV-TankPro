# Changelog

## v0.2.0
### Hardware (Controller)
- Hardware reference remains v3.0; refreshed docs for wiring/install and pairing expectations.

### Firmware (Controller)
- ESPHome controller firmware v0.2.0 with Wi-Fi and Direct (ESP-NOW) comms active and packaged for maintainability.
- Direct-mode lock preserved; factory reset returns to provisioning.
- Live telemetry (level/temp/faults) plus relay control and config over both transports.
- Link/failsafe handling improved to shut off valve when comms lost.

### Firmware (Display)
- CYD display firmware v0.2.0 with pairing/role assignment for up to two controllers.
- Shows live level/temp/fault status and issues fill/drain/clear-fault/restart commands over Wi-Fi or Direct.

## v0.1.0 (firmware)
- ESP32-C3 controller firmware promoted to production profile with refined LED states, pairing UX, relay/pair-button safeguards, and reset/fault signalling.
- CYD display firmware exchanges live data with controllers, pushes config/commands (fill, drain, restart, clear faults), and bumps the on-device label to v0.1.0.
- Removed the legacy `tankpros3.yaml` profile in favour of the C3 build; `tankpro_basic.yaml` remains for minimal S3 I/O.

## v3.0.0 (hardware)
- Major architecture redesign centred on ESP32-C3 with improved automotive power handling.
- Dual sensor front-ends tuned for 0–190 Ω and 33–240 Ω ranges.
- Added power path flexibility and clearer documentation.
- Introduced new documentation set and repository structure.

## v2.x (legacy hardware)
- Previous hardware revisions archived in `hardware/legacy/v1-v2/` for reference only; new builds should follow v3.0.
