# Firmware – Display (Placeholder)

This directory is reserved for a future external display that will show tank levels and relay status from SmartRV TankPro v3.0.

## Planned approach
- **Hardware:** Likely an ESP32-based board with a small color LCD/TFT or e-paper panel and rotary/button input.
- **Connectivity:** Wi‑Fi connection to the controller’s ESPHome API or a wired UART link (TX=GPIO21, RX=GPIO20 on the controller header).
- **Features (planned):**
  - Live tank percentage display for one or two tanks.
  - Pump/valve control via on-screen button or hardware controls.
  - Alarm indication for low/high tank thresholds.

No firmware is provided yet; this folder acts as a placeholder for future development.
