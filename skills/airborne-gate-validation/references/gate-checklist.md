# Gate Checklist

## Preflight
- Confirm board serial device (for example `/dev/cu.usbmodem211301`).
- Confirm firmware image to validate (`firmware_mpy` or `firmware_sdk`).
- Keep robot disarmed unless a test step explicitly requires `arm`.

## Baseline Checks
1. Boot telemetry appears and includes `mode` + `armed`.
2. `ping` command returns `{"type":"ack","ok":true}`.
3. `version` returns firmware identity.
4. Safety sequence (`disarm -> arm -> stop -> disarm`) returns all acks.

## Encoder Checks
1. Idle window: counts stable.
2. Left forward/backward windows: sign flips, right channel remains stable.
3. Right forward/backward windows: sign flips, left channel remains stable.
4. Document observed sign convention and whether normalization is applied.

## Failure Handling
- Stop on first failure.
- Capture raw line showing the failure.
- Suggest only the minimal corrective action before retry.
