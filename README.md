# Airborne Classic (Aurbotstem Academy)

XRP-style robot car firmware stack for STEM learning, currently focused on USB serial bring-up and deterministic RP2040 control.

## Current Status (February 21, 2026)
- Gate P1-0: PASS (MicroPython deploy, boot telemetry, `ping` ack, DISARMED-by-default).
- Gate P1-1: PASS (IRQ encoder counting validated with wheel-direction tests).
- PicoSDK track: active and validated (build/flash/smoke PASS, PIO encoder telemetry integrated).

## Repository Layout
- `firmware_mpy/`: MicroPython firmware (`main.py`, `lib/hw.py`, `config.json`).
- `firmware_sdk/`: PicoSDK firmware with NDJSON loop and PIO encoder sampler.
- `protocol/`: NDJSON protocol spec + JSON schema.
- `tools/`: host utilities (`mpy_sync.sh`, CAD bbox extraction).
- `cad/`: chassis/wheel 3MF artifacts and extracted metadata.
- `docs/`: phase status and project notes.
- `skills/`: reusable Codex skills for this repo.
- `MEMORY.md`: persistent decisions and validated environment facts.

## Quick Start: MicroPython Path
```bash
python3 -m pip install --upgrade mpremote
./tools/mpy_sync.sh
```
Expected behavior: boot telemetry in `DISARMED` mode, then `{"type":"ack","id":"1","ok":true}` for `ping`.

## Quick Start: PicoSDK Path
Install local dependencies (one-time):
```bash
git clone --depth 1 https://github.com/raspberrypi/pico-sdk.git third_party/pico-sdk
git -C third_party/pico-sdk submodule update --init --recursive
curl -fL -o /tmp/arm-gnu-toolchain.tar.xz https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi.tar.xz
tar -xf /tmp/arm-gnu-toolchain.tar.xz -C third_party
```

Then build:
```bash
export PICO_SDK_PATH="$PWD/third_party/pico-sdk"
export PICO_TOOLCHAIN_PATH="$PWD/third_party/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi/bin"
cmake -S firmware_sdk -B firmware_sdk/build-pico-arm -G Ninja
cmake --build firmware_sdk/build-pico-arm -j
```
Flash UF2: copy `firmware_sdk/build-pico-arm/airborne_classic_fw.uf2` to `RPI-RP2`.

## Source of Truth
- Hardware: `HARDWARE_MAP.md`
- Serial protocol: `protocol/ndjson_v1.md`
- Latest phase evidence: `docs/PHASE1_STATUS.md`
