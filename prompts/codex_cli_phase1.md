# Codex CLI — Airborne Classic (Phase 1) Master Prompt

Paste into codex CLI.

## Mission
Build Phase 1 (USB-only) for **Airborne Classic**: MicroPython bring-up track (auto-run) + PicoSDK production track (PIO encoders) + local-only teacher learning hub.

## Non-negotiables
- Gate-based development. Stop on FAIL.
- Source-of-truth hardware in `HARDWARE_MAP.md`.
- NDJSON protocol v1 in `protocol/ndjson_v1.md`.

## Repository
Root: `airborne-classic/`

## Gate P1-0 (deliver now)
1) Confirm `HARDWARE_MAP.md` exists and is consistent.
2) Implement MicroPython auto-run skeleton:
   - `firmware_mpy/main.py`
   - `firmware_mpy/lib/hw.py`
   - `firmware_mpy/config.json`
   - `tools/mpy_sync.sh`
3) Provide a tiny verification transcript:
   - `mpy_sync.sh`
   - `echo '{"type":"cmd","id":"1","ts":0,"cmd":"ping"}' | <serial>` (or via mpremote repl)

## Gate P1-1 (next)
- Expand MicroPython to include encoder IRQ counting for slow speed.

## PicoSDK track (do not start until MP gates pass)
- Create `firmware_sdk/` skeleton with:
  - quadrature PIO plan
  - build instructions

## Output format
- Print PASS/FAIL for Gate P1-0 with evidence.
- If FAIL, provide debug checklist.
