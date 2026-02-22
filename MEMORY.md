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

## Current Priorities
1. Calibrate PicoSDK `COUNTS_PER_WHEEL_REV_*` constants against measured wheel turns.
2. Add `set_rpm` command handling in both firmware tracks.
3. Add IMU calibration and encoder calibration command handlers.
