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
- RPM estimation constants in PicoSDK path still need final calibration to physical wheel counts-per-rev.
