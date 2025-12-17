# Roadmap

This roadmap tracks user-visible work for SmartRV TankPro. Status labels are documented below; contribute ideas via GitHub Issues/Discussions.

## Themes
- Pairing & roles (Wi‑Fi + Direct ESP‑NOW)
- Faults & safety (failsafe, leak/freeze/overfill handling)
- UI polish (CYD display, LEDs)
- Home Assistant integration scope
- Install/UX (provisioning, pairing clarity)
- OTA/maintenance

## Now (in progress)
- Stabilise Direct ESP‑NOW comms and link health/failsafe handling.
- Improve pairing/role assignment reliability and status feedback.

## Next (planned)
- Better CYD diagnostics for connectivity and faults.
- Simplified controller reset/re-pair flow documentation.
- Optional HA discovery templates.

## Later (ideas)
- Additional tank sensor types (e.g., ultrasonic) if demand exists.
- Advanced scheduling/automation hooks from CYD.
- Extended theme/UX customisation.

## How to suggest features
- Open a GitHub Issue/Discussion with a clear problem statement and desired outcome.
- Tag with labels where possible:
  - `type:feature`, `type:bug`, `type:docs`
  - `status:triage`, `status:planned`, `status:in-progress`, `status:beta-needed`
  - `area:controller-fw`, `area:display-fw`, `area:hardware`, `area:docs`
- Include mode (Wi‑Fi/Direct), firmware versions, and steps to reproduce or a mockup.
