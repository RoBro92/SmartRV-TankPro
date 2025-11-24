# Design Rationale – v3.0

## Why v3.0 vs v2.x
v3.0 is a ground-up redesign focused on robust power handling, cleaner ADC front-ends, and easier manufacturing. Improvements include hardened automotive-style power input, range-optimised sensor dividers for higher ADC resolution, ESD/transient protection, and footprints tuned for assembly houses like JLCPCB. Documentation and test points aim to simplify debugging and field maintenance.

## Power design decisions
- **Separate 5 V and 3.3 V rails:** The buck-derived 5 V rail powers the relay, LEDs, and accessories, while a 3.3 V LDO feeds the ESP32-C3 and ADC front-ends for stable measurements.
- **Dual input (RV DC and USB-C):** RV DC is the primary operating supply; USB-C allows firmware flashing and bench validation without the vehicle harness.
- **LM66200 vs Schottky OR-ing:** The PCB allows either an LM66200 ideal-diode mux (minimal drop/heat, higher cost) or a simpler dual-Schottky OR-ing (SS34/SS54) when voltage headroom allows. Builders can choose per BOM and availability.

## Sensor front-end
- **Range-tailored dividers:** Separate resistor dividers are used for the 0–190 Ω and 33–240 Ω ranges so each channel maximises ADC dynamic range.
- **Series protection resistor (~1 kΩ):** Limits fault current into the ADC pin and improves EMI/ESD robustness.
- **Local 100 nF capacitor:** Placed close to the ESP32 ADC pins to smooth sampled voltage and reduce noise on long runs.
- **ESD/TVS protection:** Diode from the sense node to ground near the connector clamps transients common in RV wiring harnesses.

## Grounding and layout
- **Ground pours on both layers:** Critical components (ESP32, regulators, sensor front-ends) reference low-impedance ground with via stitching between planes.
- **Solid ground connections where needed:** Power components, decoupling caps, and high-current returns favour solid pours over thermal reliefs; less critical parts may use thermals to ease soldering.
- **Ground vias near the ESP32 module:** Ground pads and the RF section can include stitched vias to improve return paths and RF behaviour.

## Protection and outputs
- **Relay driver:** A transistor drives the 5 V relay coil; a flyback diode (e.g., SS14) protects the transistor and reduces EMI.
- **Buzzer drive:** Passive buzzer uses a transistor stage; optional 100 kΩ pull-down on the control pin keeps the state defined at boot.
- **Reset/boot controls:** EN/RESET and BOOT buttons follow Espressif reference guidance. Optional capacitors on these lines may be DNP if stability is proven without them.
