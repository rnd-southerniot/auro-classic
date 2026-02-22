# firmware_sdk

PicoSDK production firmware for Airborne Classic.

## Scope
Current implementation provides:
- USB NDJSON command/telemetry loop in `src/main.c`
- PIO-based quadrature sampling (`src/quadrature_sample.pio`)
- encoder count telemetry fields (`enc_l`, `enc_r`)

## Prerequisites
- Pico SDK checkout
- Arm GNU toolchain with `arm-none-eabi-*`
- CMake + Ninja

Tested local setup:
- `PICO_SDK_PATH=$PWD/third_party/pico-sdk`
- `PICO_TOOLCHAIN_PATH=$PWD/third_party/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi/bin`

## Install Local Dependencies (one-time)
```bash
git clone --depth 1 https://github.com/raspberrypi/pico-sdk.git third_party/pico-sdk
git -C third_party/pico-sdk submodule update --init --recursive
curl -fL -o /tmp/arm-gnu-toolchain.tar.xz https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi.tar.xz
tar -xf /tmp/arm-gnu-toolchain.tar.xz -C third_party
```

## Build
```bash
cmake -S firmware_sdk -B firmware_sdk/build-pico-arm -G Ninja
cmake --build firmware_sdk/build-pico-arm -j
```

## Flash
1. Reboot to BOOTSEL (`picotool reboot -u -f`) or hold BOOTSEL while plugging USB.
2. Copy `firmware_sdk/build-pico-arm/airborne_classic_fw.uf2` to `RPI-RP2`.

## Protocol Coverage
Implemented command subset:
- `ping`, `version`, `arm`, `disarm`, `set_pwm`, `set_rpm`, `stop`

`set_rpm` currently maps target RPM values to open-loop PWM using `OPEN_LOOP_MAX_ABS_RPM` in `src/main.c`.

Telemetry:
- `mode`, `armed`, `pwm_l`, `pwm_r`, `enc_l`, `enc_r`, `rpm_l`, `rpm_r`, `yaw`, `fault`
- `rpm_*` is estimated from encoder deltas each telemetry interval.
- `COUNTS_PER_WHEEL_REV_L/R` in `src/main.c` are currently calibrated to `582/583` from Feb 22, 2026 bench trials.

## Notes
- Pin mapping remains anchored in `HARDWARE_MAP.md`.
- Protocol compatibility target remains `protocol/ndjson_v1.md`.
- Encoder sign is normalized in telemetry using `ENC_L_SIGN` / `ENC_R_SIGN` in `src/main.c`.
