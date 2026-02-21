#!/usr/bin/env python3
import argparse
import json
import time

import serial


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Capture RP2040 NDJSON serial traffic.")
    p.add_argument("--port", required=True, help="Serial port path, e.g. /dev/cu.usbmodem211301")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--duration", type=float, default=3.0)
    p.add_argument("--send", action="append", default=[], help="Raw JSON line to send before capture (repeatable)")
    p.add_argument("--summary", action="store_true", help="Print telemetry delta summary")
    return p.parse_args()


def main() -> int:
    args = parse_args()
    ser = serial.Serial(args.port, args.baud, timeout=0.25)
    ser.reset_input_buffer()

    for line in args.send:
        ser.write((line + "\n").encode())
        ser.flush()
        time.sleep(0.08)

    end = time.time() + args.duration
    lines = []
    tele = []

    while time.time() < end:
        raw = ser.readline()
        if not raw:
            continue
        text = raw.decode("utf-8", "replace").strip()
        if not text:
            continue
        lines.append(text)
        try:
            obj = json.loads(text)
        except Exception:
            continue
        if obj.get("type") == "tele":
            tele.append((obj.get("ts"), obj.get("enc_l"), obj.get("enc_r")))

    ser.close()

    for line in lines:
        print(line)

    if args.summary and tele:
        start = tele[0]
        endt = tele[-1]
        print("summary_samples", len(tele))
        print("summary_start", start)
        print("summary_end", endt)
        print("summary_delta_l", endt[1] - start[1])
        print("summary_delta_r", endt[2] - start[2])

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
