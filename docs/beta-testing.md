# Beta testing (5-board giveaway)

We’re preparing a small beta (five kits) to gather real-world feedback before v0.3.0.

## What TankPro is
ESP32-C3 tank controller(s) plus a Cheap Yellow Display (CYD) that shows levels/faults and controls fill/drain over Wi‑Fi or Direct ESP‑NOW.

## What testers may receive
- **Controller-only kit:** 1x controller PCB assembled, wiring pigtails (depending on cost).
- **Full kit (if available):** Controller + CYD display + wiring pigtails. Availability depends on budget.

## Eligibility & expectations
- Comfortable with 12–24 V wiring and basic DIY (no mains).
- Can install on an RV/boat tank and test for at least 2 weeks.
- Will share feedback/logs/photos/videos. Safety first; stop if unsure.

## Setup (high-level)
- Hardware overview: `hardware/controller/README.md`
- Controller firmware flow: `firmware/src/controller/README.md`
- CYD firmware/UI: `firmware/src/display/README.md`

## Feedback process
- File Issues with label `status:beta-needed` plus `type:bug`/`type:feature`/`type:docs` and `area:*` as appropriate.
- Include firmware mode (Wi‑Fi/Direct), versions, steps, logs, and photos/videos if possible.
- Short weekly note is ideal; faster for critical faults.

## Safety & privacy
- 12–24 V DC only; avoid water ingress; validate valve current within specs.
- No data is collected automatically. Logs are only what you share.

## Exit criteria (v0.3.0 readiness)
- Stable comms (Wi‑Fi + Direct), clear pairing/reset flows, reliable fault handling, and beta feedback addressed or triaged.
