#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
SDK_PATH="${PICO_SDK_PATH:-$ROOT_DIR/third_party/pico-sdk}"
TOOLCHAIN_PATH="${PICO_TOOLCHAIN_PATH:-$ROOT_DIR/third_party/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi/bin}"
BUILD_DIR="${1:-$ROOT_DIR/firmware_sdk/build-pico-arm}"

PICO_SDK_PATH="$SDK_PATH" PICO_TOOLCHAIN_PATH="$TOOLCHAIN_PATH" cmake -S "$ROOT_DIR/firmware_sdk" -B "$BUILD_DIR" -G Ninja
PICO_SDK_PATH="$SDK_PATH" PICO_TOOLCHAIN_PATH="$TOOLCHAIN_PATH" cmake --build "$BUILD_DIR" -j

echo "Build complete: $BUILD_DIR"
