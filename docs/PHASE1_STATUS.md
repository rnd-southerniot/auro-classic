# Phase 1 Status (As of February 22, 2026)

## Gate Results
- P1-0: PASS
- P1-1: PASS
- PicoSDK smoke gate: PASS

## Verified Behaviors
- MicroPython path:
  - deploy via `./tools/mpy_sync.sh`
  - boot telemetry in `DISARMED`
  - `ping` ack success
  - encoder IRQ counting with direction-dependent deltas
- PicoSDK path:
  - deterministic build with local SDK/toolchain
  - UF2 flash to RP2040
  - NDJSON command loop acks for `ping/version/arm/disarm/set_pwm/set_rpm/stop`
  - PIO-derived encoder telemetry (`enc_l`, `enc_r`)

## Build Environment Used
- `PICO_SDK_PATH=/Users/robotics/airborne-classic/third_party/pico-sdk`
- `PICO_TOOLCHAIN_PATH=/Users/robotics/airborne-classic/third_party/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi/bin`
- Board serial: `/dev/cu.usbmodem211301`

## Known Gaps
- Calibration commands are not implemented yet (`cal_imu`, `cal_encoders`).
- Closed-loop RPM control is not implemented yet (`set_rpm` is open-loop mapping).

## Calibration Snapshot (February 22, 2026)
- Left wheel counts/rev trials: `576`, `586`, `585` -> average `582.33` -> configured `582`.
- Right wheel counts/rev trials: `575`, `591`, `584` -> average `583.33` -> configured `583`.
