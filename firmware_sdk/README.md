# firmware_sdk

PicoSDK production-track firmware skeleton for Airborne Classic.

## Scope
This track starts only after MicroPython gates pass. The current skeleton provides:
- a minimal Pico SDK build target,
- a runtime entrypoint (`src/main.c`) with NDJSON command/telemetry loop,
- a quadrature PIO implementation plan (`docs/pio_quadrature_plan.md`).

## Prerequisites
- Raspberry Pi Pico SDK installed.
- `PICO_SDK_PATH` exported.
- CMake 3.13+ and ARM GCC toolchain.

## Build
```bash
cmake -S firmware_sdk -B firmware_sdk/build
cmake --build firmware_sdk/build -j
```

## Flash (UF2)
1. Hold BOOTSEL and connect RP2040 board.
2. Copy `firmware_sdk/build/airborne_classic_fw.uf2` to the mounted RP2040 drive.

## Notes
- Pin mapping and hardware truth remain anchored in `HARDWARE_MAP.md`.
- Protocol compatibility target is `protocol/ndjson_v1.md`.
- PIO quadrature details and milestones are documented in `docs/pio_quadrature_plan.md`.
- Current `src/main.c` supports: `ping`, `version`, `arm`, `disarm`, `set_pwm`, `stop`.
- Encoder counts (`enc_l`, `enc_r`) are sourced from PIO-based encoder samplers on GP2-3 and GP4-5.
- RPM output is still a placeholder (`rpm_l/rpm_r=0.0`) until count-to-RPM conversion is added.
