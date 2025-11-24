# Wiring and Installation

Follow these guidelines to safely wire SmartRV TankPro v3.0 into a 12–24 V RV/boat system.

## Power connections
- **VIN (12–24 V DC):** Primary supply input from the RV electrical system. 
- **GND:** System ground. Tie to the vehicle negative bus.
- **USB-C:** Secondary/power for flashing and bench testing; can also power the board without VIN connected.

## Connector pin-outs (ESP32-C3 mapping)
- **Tank Level:** Sense line → GPIO0 (ADC). Other sensor leg to GND.
- **Relay output:** Relay control transistor → GPIO10. Relay contacts (NO/COM) switch the external pump/valve on the 5 V rail.
- **WS2812C LED chain:** Data in → GPIO8.
- **Buzzer (optional):** Drive → GPIO9.
- **UART header (future display/debug):** TX → GPIO21, RX → GPIO20 (3.3 V logic), plus 3V3 and GND.

## Example wiring scenarios
- **Single tank (0–190 Ω):**
  - Connect sensor wiper to Sensor 1 input (GPIO0), sensor return to GND.
- 
- **Relay control:**
  - Wire VIN (5–24 V) to relay COM, pump/valve lead to relay NO, and the other lead of the pump/valve to GND. Verify relay contact voltage/current limits.

## Installation notes
- Keep sensor cable runs short where possible; if long, consider shielded cable and tie shield to GND at the module end.
- Route sensor wiring away from high-current lines (inverters, alternators) to reduce noise.
- Ensure polarity protection/fusing on VIN. Confirm all connectors are tightened before powering up.
- Use strain relief on USB-C and sensor/relay cables to avoid stressing the PCB connectors.
