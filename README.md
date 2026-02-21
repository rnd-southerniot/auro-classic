# Airborne Classic (Aurbotstem Academy)

XRP-style robot car + custom controller + web learning hub for STEM students.

## Phase Plan

### Phase 1 (USB-only)
- MicroPython bring-up track (auto-run `main.py`), deployed via `mpremote`
- PicoSDK production track (PIO quadrature), deterministic control loops
- Local-only learning hub (teacher laptop)

### Phase 2 (after Phase 1 PASS)
- Wi-Fi/BLE + Android/iOS app
- Multi-robot classroom mode

## Hardware (current)
- Controller: Cytron Maker Pi RP2040
- IMU: ICM-20948
- Motors: XRP-style DC gear motors w/ quadrature encoders

## Notes captured
- Motor driver pins (Maker Pi RP2040):
  - M1A=GP8, M1B=GP9, M2A=GP10, M2B=GP11
  - Truth table: 00 brake, 10 forward, 01 backward, 11 coast
- WS2812: GP18
- Buttons: GP20/GP21
- Buzzer: GP22

## Next
- Locate XRP-like chassis CAD/STL/STEP files (wheel diameter, wheelbase, track width).
- Build `HARDWARE_MAP.md` for encoders + IMU pin mapping.
