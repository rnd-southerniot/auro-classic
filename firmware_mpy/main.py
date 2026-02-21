"""Airborne Classic (MicroPython) — auto-run entrypoint.

Phase 1 bring-up track:
- DISARMED by default
- NDJSON over USB serial (stdin/stdout)

Gates:
- MP-0: boot banner + ping/version
- MP-1: I2C scan (IMU)
- MP-2: open-loop motor control
- MP-3: encoder counts (IRQ-based)

This is intentionally minimal; deeper control moves to PicoSDK track.
"""

import json
import sys
import time

try:
    import uselect as select
except ImportError:
    import select

from lib.hw import Hardware


def now_ms() -> int:
    return int(time.time() * 1000)


LOG_ON = False


def flush_stdout():
    try:
        sys.stdout.flush()
    except AttributeError:
        pass


def println(obj):
    sys.stdout.write(json.dumps(obj) + "\n")
    flush_stdout()


def log(msg: str):
    global LOG_ON
    if not LOG_ON:
        return
    sys.stdout.write("# " + str(msg) + "\n")
    flush_stdout()


def tele_payload(hw: Hardware, include_meta: bool = False):
    enc_l, enc_r = hw.get_encoder_counts()
    pwm_l, pwm_r = hw.get_pwm()
    payload = {
        "type": "tele",
        "ts": now_ms(),
        "mode": "ARMED" if hw.armed else "DISARMED",
        "armed": hw.armed,
        "pwm_l": pwm_l,
        "pwm_r": pwm_r,
        "enc_l": enc_l,
        "enc_r": enc_r,
        "rpm_l": 0.0,
        "rpm_r": 0.0,
        "yaw": 0.0,
        "fault": None,
    }
    if include_meta:
        payload["fw"] = hw.fw_version
        payload["robot"] = hw.robot_name
    return payload


def main():
    hw = Hardware.from_config("config.json")
    tele_hz = max(1, int(hw.telemetry_hz))
    tele_period_ms = max(1, int(1000 / tele_hz))
    next_tele_ms = now_ms()

    poller = select.poll()
    poller.register(sys.stdin, select.POLLIN)

    println(tele_payload(hw, include_meta=True))
    log(f"boot: robot={hw.robot_name} fw={hw.fw_version}")

    while True:
        if now_ms() >= next_tele_ms:
            println(tele_payload(hw))
            next_tele_ms = now_ms() + tele_period_ms

        line = ""
        if poller.poll(10):
            line = sys.stdin.readline()
        if not line:
            continue
        line = line.strip()
        if not line:
            continue

        try:
            msg = json.loads(line)
        except Exception as e:
            println({"type": "ack", "id": "?", "ok": False, "error": {"code": "bad_json", "message": str(e)}})
            continue

        if msg.get("type") != "cmd":
            println({"type": "ack", "id": msg.get("id", "?"), "ok": False, "error": {"code": "bad_type", "message": "expected cmd"}})
            continue

        cmd = msg.get("cmd")
        cmd_id = msg.get("id", "?")

        try:
            if cmd == "ping":
                println({"type": "ack", "id": cmd_id, "ok": True})
            elif cmd == "set_log":
                global LOG_ON
                LOG_ON = bool(msg.get("on", False))
                println({"type": "ack", "id": cmd_id, "ok": True, "data": {"log": LOG_ON}})
                log(f"log: set_log={LOG_ON}")
            elif cmd == "version":
                println({"type": "ack", "id": cmd_id, "ok": True, "data": {"fw": hw.fw_version, "robot": hw.robot_name}})
            elif cmd == "i2c_scan":
                addrs = hw.i2c_scan()
                println({"type": "ack", "id": cmd_id, "ok": True, "data": {"addrs": addrs}})
            elif cmd == "arm":
                hw.arm(True)
                println({"type": "ack", "id": cmd_id, "ok": True})
            elif cmd == "disarm":
                hw.arm(False)
                println({"type": "ack", "id": cmd_id, "ok": True})
            elif cmd == "stop":
                hw.stop()
                println({"type": "ack", "id": cmd_id, "ok": True})
            elif cmd == "set_pwm":
                left = float(msg.get("left", 0.0))
                right = float(msg.get("right", 0.0))
                hw.set_pwm(left, right)
                println({"type": "ack", "id": cmd_id, "ok": True})
            else:
                println({"type": "ack", "id": cmd_id, "ok": False, "error": {"code": "bad_cmd", "message": f"unknown cmd {cmd}"}})

        except Exception as e:
            println({"type": "ack", "id": cmd_id, "ok": False, "error": {"code": "exception", "message": str(e)}})


if __name__ == "__main__":
    main()
