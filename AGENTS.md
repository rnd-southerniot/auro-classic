# Repository Guidelines

## Project Structure & Module Organization
- `firmware_mpy/`: active MicroPython firmware. Entry point is `firmware_mpy/main.py`; hardware abstraction is `firmware_mpy/lib/hw.py`; board/runtime config is `firmware_mpy/config.json`.
- `firmware_sdk/src/`: reserved for upcoming PicoSDK production firmware (currently scaffold-only).
- `protocol/`: NDJSON contract (`ndjson_v1.md`) and JSON schema (`ndjson_v1.schema.json`) shared by host and controller.
- `tools/`: developer utilities, including device sync (`mpy_sync.sh`) and CAD dimension extraction (`extract_3mf_bbox.py`).
- `cad/`, `HARDWARE_MAP.md`: mechanical references and locked hardware facts.
- `docs/`, `calibration/`: documentation/calibration placeholders; add new artifacts here rather than ad-hoc locations.

## Build, Test, and Development Commands
- `./tools/mpy_sync.sh`: installs/updates `mpremote`, copies MicroPython files to the RP2040, and soft-resets.
- `mpremote connect auto repl`: open REPL after sync.
- `python3 tools/extract_3mf_bbox.py cad/extracted/frame/3D/3dmodel.model`: compute CAD bounding boxes for geometry checks.
- Manual protocol smoke test (USB serial): send `{"type":"cmd","id":"1","ts":0,"cmd":"ping"}` and expect `{"type":"ack","id":"1","ok":true}`.

## Coding Style & Naming Conventions
- Python: 4-space indentation, `snake_case` for functions/variables, `PascalCase` for classes, concise docstrings where behavior is non-obvious.
- Keep MicroPython code dependency-light (`machine`, `time`, `json`, `sys`) and deterministic in control paths.
- Bash: `#!/usr/bin/env bash` plus `set -euo pipefail` (as in `tools/mpy_sync.sh`).
- File/module names: lowercase with underscores (example: `extract_3mf_bbox.py`).

## Testing Guidelines
- Current project phase uses hardware-in-the-loop checks instead of automated suites.
- Minimum gate before merge:
  - firmware sync succeeds,
  - `ping`, `version`, and `i2c_scan` commands return valid `ack` payloads,
  - robot remains `DISARMED` by default on boot.
- Keep protocol changes backward-compatible and update both `protocol/ndjson_v1.md` and `protocol/ndjson_v1.schema.json` in the same PR.

## Commit & Pull Request Guidelines
- Git history is not available in this workspace snapshot; use Conventional Commit style (`feat:`, `fix:`, `docs:`, `chore:`) for consistency.
- Keep commits focused (firmware vs protocol vs CAD tooling separated).
- PRs should include:
  - scope summary and affected paths,
  - hardware used (board/sensors),
  - brief PASS/FAIL gate evidence (key command outputs),
  - linked issue/task when applicable.
