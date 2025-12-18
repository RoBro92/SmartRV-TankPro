# Firmware – Display (CYD, v0.2.0)

LVGL firmware for the Cheap Yellow Display (ESP32-2432S028). Acts as the primary node for up to two TankPro controllers.

## What it does
- Discovers controllers, assigns Fresh/Waste roles, and stores peer MAC/keys.
- Shows live level, temperature, leak/fault status, and relay state.
- Issues commands: Fill, Drain, Clear Faults, Restart, and config updates.
- Works over Wi‑Fi (UDP) or Direct (ESP‑NOW) depending on controller mode.

## UI actions
- **Pair/assign roles:** Open Assign Roles overlay, select a discovered controller, choose Fresh/Waste.
- **Control:** Use Fill/Drain buttons per role; Clear Faults to reset alarms; Restart to reboot controller.
- **View status:** Level %, temperature, leak/fault text, and connection state per role.

## UI snapshots

<table>
  <tr>
    <td><img src="../../hardware/media/images/cydhome.JPG?raw=1" alt="CYD home" width="320"/></td>
    <td><img src="../../hardware/media/images/cydhomepair.JPG?raw=1" alt="CYD home pairing prompt" width="320"/></td>
  </tr>
  <tr>
    <td><img src="../../hardware/media/images/cydadd.JPG?raw=1" alt="Assign/Add screen" width="320"/></td>
    <td><img src="../../hardware/media/images/cydtankdetails.JPG?raw=1" alt="Tank overlay/details" width="320"/></td>
  </tr>
</table>

## Connection modes
- Wi‑Fi: CYD and controllers on same network; uses UDP for state/commands.
- Direct: CYD uses ESP‑NOW on channel 6 to talk to paired controllers (max two).

## Build/flash
- Option A (prebuilt, v0.2.0): flash the release binaries in `releases/v0.2.0/` (`cyd-bootloader-0.2.0.bin`, `cyd-partitions-0.2.0.bin`, `cyd-firmware-0.2.0.bin`) with esptool/PlatformIO upload.
- Option B (build): from `firmware/src/display/CYD` run `pio run -e cyd` (or `pio run -t upload -e cyd --upload-port <port>` to flash).
- Binaries output to `.pio/build/cyd/` (bootloader, partitions, firmware) if you build locally.
- Why three files? esptool needs bootloader + partitions + firmware; PlatformIO upload handles packing automatically if you just use `pio run -t upload`.
- If needed, hold BOOT and tap RST to enter bootloader before upload.
- Version shown on UI labels: **v0.2.0**.

Docs/index/roadmap: see `docs/README.md`.
Images/assets: see `docs/assets/` (for any shared UI images).

## Changelog
See `CHANGELOG.md` in this directory.
