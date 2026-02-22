# NDJSON Serial Protocol v1 (Airborne Classic)

Transport: USB CDC serial
Encoding: newline-delimited JSON (one JSON object per line)

## Commands (Hub → Controller)

All commands include:
- `type: "cmd"`
- `id`: string UUID
- `ts`: epoch milliseconds
- `cmd`: command name

### Arm/disarm
```json
{"type":"cmd","id":"uuid","ts":0,"cmd":"arm"}
{"type":"cmd","id":"uuid","ts":0,"cmd":"disarm"}
```

### Motor control
Open loop PWM (normalized -1..1):
```json
{"type":"cmd","id":"uuid","ts":0,"cmd":"set_pwm","left":0.2,"right":0.2}
```

RPM command:
```json
{"type":"cmd","id":"uuid","ts":0,"cmd":"set_rpm","left":120,"right":120}
```
Current firmware subsets map RPM targets to open-loop PWM; closed-loop control is a later phase.

Emergency stop:
```json
{"type":"cmd","id":"uuid","ts":0,"cmd":"stop"}
```

### Calibration
```json
{"type":"cmd","id":"uuid","ts":0,"cmd":"cal_imu"}
{"type":"cmd","id":"uuid","ts":0,"cmd":"cal_encoders"}
```

### Introspection
```json
{"type":"cmd","id":"uuid","ts":0,"cmd":"ping"}
{"type":"cmd","id":"uuid","ts":0,"cmd":"i2c_scan"}
{"type":"cmd","id":"uuid","ts":0,"cmd":"version"}
```

### Logging
Default is **NDJSON only**. Human logs can be enabled for debugging.

```json
{"type":"cmd","id":"uuid","ts":0,"cmd":"set_log","on":true}
{"type":"cmd","id":"uuid","ts":0,"cmd":"set_log","on":false}
```

Human log lines are prefixed with `# ` so parsers can ignore them.

## Telemetry (Controller → Hub)

Telemetry at 10–50 Hz depending mode.

```json
{
  "type":"tele",
  "ts":0,
  "mode":"DISARMED|ARMED|TELEOP|AUTO|FAULT",
  "armed":false,
  "pwm_l":0.0,
  "pwm_r":0.0,
  "enc_l":0,
  "enc_r":0,
  "rpm_l":0.0,
  "rpm_r":0.0,
  "yaw":0.0,
  "fault":null
}
```

## Acks (Controller → Hub)

```json
{"type":"ack","id":"uuid","ok":true}
```

Error:
```json
{"type":"ack","id":"uuid","ok":false,"error":{"code":"bad_cmd","message":"..."}}
```

## Implementation Status (February 22, 2026)

Protocol v1 is the contract; firmware tracks it in staged subsets.

- MicroPython (`firmware_mpy/main.py`) currently implements:
  - `ping`, `set_log`, `version`, `i2c_scan`, `arm`, `disarm`, `stop`, `set_pwm`, `set_rpm` (open-loop mapping)
- PicoSDK (`firmware_sdk/src/main.c`) currently implements:
  - `ping`, `version`, `arm`, `disarm`, `stop`, `set_pwm`, `set_rpm` (open-loop mapping)
- Deferred in both tracks (planned later gates):
  - `cal_imu`, `cal_encoders`
