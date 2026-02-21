---
name: pico-sdk-build-flash
description: Build, flash, and smoke-test Airborne Classic PicoSDK firmware on RP2040 using reproducible local paths and UF2 workflows. Use when configuring toolchain paths, running CMake/Ninja builds, rebooting to BOOTSEL, copying UF2 images, and verifying NDJSON telemetry/ack behavior after flash.
---

# Pico SDK Build Flash

## Use Deterministic Build Paths
1. Prefer repository-local toolchain and SDK paths from `references/build-paths.md`.
2. Configure with Ninja and an explicit build directory (`firmware_sdk/build-pico-arm`).
3. Re-run configure when CMake cache uses a different generator.

## Build Then Flash
1. Run `scripts/build_sdk.sh` first.
2. Run `scripts/flash_uf2.sh` with the UF2 path.
3. If copy fails on first attempt, retry once after mount confirms writable BOOTSEL volume.

## Smoke-Test Immediately
- Send `ping` and `version` commands and verify acks.
- Confirm telemetry stream appears without parse errors.
- Check encoder fields are present (`enc_l`, `enc_r`).

## Resources
- `references/build-paths.md`: canonical env/path values and fallback notes.
- `scripts/build_sdk.sh`: deterministic build wrapper.
- `scripts/flash_uf2.sh`: BOOTSEL flash helper.
