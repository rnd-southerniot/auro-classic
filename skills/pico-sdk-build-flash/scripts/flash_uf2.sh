#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <path-to-uf2>" >&2
  exit 2
fi

UF2_PATH="$1"
MOUNT_POINT="/Volumes/RPI-RP2"

if [[ ! -f "$UF2_PATH" ]]; then
  echo "UF2 not found: $UF2_PATH" >&2
  exit 2
fi

picotool reboot -u -f || true

for _ in $(seq 1 40); do
  if [[ -d "$MOUNT_POINT" ]]; then
    break
  fi
  sleep 0.25
done

if [[ ! -d "$MOUNT_POINT" ]]; then
  echo "BOOTSEL volume not found at $MOUNT_POINT" >&2
  exit 1
fi

cp "$UF2_PATH" "$MOUNT_POINT"/
echo "Flashed: $UF2_PATH"
