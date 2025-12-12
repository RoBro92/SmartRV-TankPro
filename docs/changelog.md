# Changelog

## v0.1.0 (firmware)
- ESP32-C3 controller firmware promoted to production profile with refined LED states, pairing UX, relay/pair-button safeguards, and reset/fault signalling.
- CYD display firmware now exchanges live data with controllers, pushes config/commands (fill, drain, restart, clear faults), and bumps the on-device label to v0.1.0.
- Removed the legacy `tankpros3.yaml` profile in favour of the C3 build; `tankpro_basic.yaml` remains for minimal S3 I/O.

## v3.0.0
- Major architecture redesign centred on ESP32-C3 with improved automotive power handling.
- Dual sensor front-ends tuned for 0–190 Ω and 33–240 Ω ranges.
- Added power path flexibility  and clearer documentation.
- Introduced new documentation set and repository structure.

## v2.x (legacy)
- Previous hardware revisions now archived in [`hardware-v2-legacy`](../hardware-v2-legacy/).
- Retained for reference only; new builds should follow v3.0.
