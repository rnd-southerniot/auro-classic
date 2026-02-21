#!/usr/bin/env bash
set -euo pipefail

# Sync MicroPython firmware to Pico using mpremote (macOS).
#
# Usage:
#   ./tools/mpy_sync.sh

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v mpremote >/dev/null 2>&1; then
  echo "ERROR: mpremote is not installed." >&2
  echo "Install it with: python3 -m pip install --upgrade mpremote" >&2
  exit 2
fi

mpremote connect auto fs mkdir :lib 2>/dev/null || true
mpremote connect auto fs cp -r "$ROOT_DIR/firmware_mpy/lib" :
mpremote connect auto fs cp "$ROOT_DIR/firmware_mpy/config.json" :config.json
mpremote connect auto fs cp "$ROOT_DIR/firmware_mpy/main.py" :main.py

# soft reset to start main.py
mpremote connect auto soft-reset

echo "Synced. Open REPL with: mpremote connect auto repl"
