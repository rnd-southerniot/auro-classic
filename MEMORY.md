# Project Memory

## Stable Decisions
- Use gate-based development with stop-on-FAIL and transcript evidence.
- Treat `HARDWARE_MAP.md` as hardware source of truth.
- Treat `protocol/ndjson_v1.md` as wire contract; staged subset implementation is allowed by gate.

## Validated Hardware/Firmware Facts
- Board: Cytron Maker Pi RP2040.
- Encoder pins: left GP2/GP3, right GP4/GP5.
- Motor pins: GP8-GP11.
- I2C bus for IMU mapping: `I2C1` on GP6/GP7.
- MicroPython runtime requires stdout flush guard (`sys.stdout.flush` may be absent).

## Validated Build/Flash Facts
- PicoSDK build works with local vendored toolchain + SDK under `third_party/`.
- Working build output path: `firmware_sdk/build-pico-arm/airborne_classic_fw.uf2`.
- Reliable BOOTSEL command flow:
  - `picotool reboot -u -f`
  - copy UF2 to `RPI-RP2`.

## Encoder Sign Convention
- Telemetry is normalized so forward is positive for both channels.
- Configured in `firmware_sdk/src/main.c` with:
  - `ENC_L_SIGN = 1`
  - `ENC_R_SIGN = -1`

## Encoder Calibration Baseline
- Date: February 22, 2026.
- Left counts/rev trials: 576, 586, 585 -> average 582.33 -> configured 582.
- Right counts/rev trials: 575, 591, 584 -> average 583.33 -> configured 583.

## Current Priorities
1. Replace open-loop `set_rpm` mapping with closed-loop RPM control.
2. Add IMU calibration and encoder calibration command handlers.
3. Add persistent calibration command flow to write/read runtime calibration values.
