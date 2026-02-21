# Quadrature PIO Plan

## Goal
Implement deterministic encoder counting on RP2040 PIO for Airborne Classic production firmware.

## Hardware Inputs
From `HARDWARE_MAP.md`:
- Left encoder: `ENC_L_A=GP2`, `ENC_L_B=GP3`
- Right encoder: `ENC_R_A=GP4`, `ENC_R_B=GP5`

## Architecture
- Use one PIO state machine per wheel encoder.
- Each state machine samples A/B, tracks 2-bit state transitions, and emits signed step deltas.
- Core loop aggregates deltas into 32-bit counts (`enc_l`, `enc_r`).

## Data Path
1. PIO captures edge transitions with fixed timing.
2. IRQ or FIFO polling transfers deltas to CPU.
3. Control task updates counts and computes low-rate RPM.
4. Telemetry publishes NDJSON-compatible fields (`enc_l`, `enc_r`, `rpm_l`, `rpm_r`).

## Failure Handling
- Invalid transitions (`00->11`, `01->10`) are counted as noise events.
- FIFO overflow increments an overflow counter and raises a soft fault.
- Encoder disconnect detection: no transitions for a configurable timeout while PWM is non-zero.

## Milestones
1. PIO state machine stub compiles and streams raw A/B states.
2. Signed delta decoding validated against hand rotation.
3. Count accumulation + overflow counters integrated.
4. Telemetry export aligned with `protocol/ndjson_v1.md`.
