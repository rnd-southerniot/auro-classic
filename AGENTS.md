# Repository Guidelines

## Project Structure & Module Organization
- `firmware_mpy/`: MicroPython runtime used for gate bring-up.
- `firmware_sdk/`: active PicoSDK firmware (`src/main.c`, `src/quadrature_sample.pio`, `docs/pio_quadrature_plan.md`).
- `protocol/`: NDJSON contract (`ndjson_v1.md`) and schema (`ndjson_v1.schema.json`).
- `tools/`: deployment and CAD helpers (`mpy_sync.sh`, `extract_3mf_bbox.py`).
- `docs/`: status and verification docs. Use this for phase reports.
- `skills/`: repository-local Codex skills.
- `MEMORY.md`: persistent operational context for future sessions.

## Build, Test, and Development Commands
- `./tools/mpy_sync.sh`: sync MicroPython files to RP2040 (requires `mpremote`).
- `mpremote connect /dev/tty.usbmodem* repl`: direct MicroPython REPL.
- `cmake -S firmware_sdk -B firmware_sdk/build-pico-arm -G Ninja`: configure PicoSDK firmware.
- `cmake --build firmware_sdk/build-pico-arm -j`: build UF2/ELF/HEX outputs.
- `picotool reboot -u -f`: force BOOTSEL for flash workflows.

## Coding Style & Naming Conventions
- Python: 4 spaces, `snake_case`, dependency-light for MicroPython paths.
- C (PicoSDK): explicit fixed-width types (`int32_t`, `uint32_t`), small static helpers, deterministic loops.
- Bash: `set -euo pipefail` and explicit error handling.
- File names: lowercase with hyphens/underscores (`quadrature_sample.pio`).

## Testing Guidelines
- Gate-based testing: stop on FAIL and include transcript evidence.
- Minimum firmware checks:
  - boot telemetry present,
  - `ping`/`version` ack success,
  - disarm/arm/stop flow behaves correctly,
  - encoder deltas follow commanded wheel direction.
- Update both protocol files together when NDJSON fields change.

## Commit & Pull Request Guidelines
- Use Conventional Commits (`feat:`, `fix:`, `docs:`, `chore:`).
- Keep commits scoped by layer (MPY, SDK, protocol, docs, tools).
- PRs should include hardware context, exact commands run, and PASS/FAIL transcript snippets.
