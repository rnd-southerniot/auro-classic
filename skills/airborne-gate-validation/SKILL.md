---
name: airborne-gate-validation
description: Run gate-based validation for Airborne Classic RP2040 firmware with stop-on-fail behavior and transcript evidence. Use when verifying MicroPython or PicoSDK firmware gates, capturing serial ack/telemetry output, checking disarm/arm safety flow, validating encoder direction deltas, or preparing PASS/FAIL gate reports.
---

# Airborne Gate Validation

## Execute Gate Workflow
1. Load the checklist in `references/gate-checklist.md` and select the target track (`firmware_mpy` or `firmware_sdk`).
2. Start with non-motion checks first: boot telemetry, `ping`, `version`, and disarm safety.
3. Stop immediately on first failure and produce a focused debug checklist.
4. Only continue motion-based checks after non-motion checks pass.

## Capture Evidence
- Use `scripts/serial_capture.py` to collect line-by-line serial evidence.
- Always include:
  - commands sent,
  - ack lines,
  - at least one telemetry sample around each command.
- For encoder checks, report `delta_l` and `delta_r` over a fixed window.

## Report Format
- Print one gate status line: `Gate <name>: PASS|FAIL`.
- If PASS: include key evidence snippets and next gate recommendation.
- If FAIL: include exact failing step, raw error line, and immediate debug actions.

## Resources
- `references/gate-checklist.md`: execution order and acceptance criteria.
- `scripts/serial_capture.py`: repeatable capture utility for NDJSON sessions.
