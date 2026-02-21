# Build Paths

## Preferred Local Paths
- `PICO_SDK_PATH=$REPO/third_party/pico-sdk`
- `PICO_TOOLCHAIN_PATH=$REPO/third_party/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi/bin`

## Configure + Build
```bash
PICO_SDK_PATH="$PWD/third_party/pico-sdk" \
PICO_TOOLCHAIN_PATH="$PWD/third_party/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi/bin" \
cmake -S firmware_sdk -B firmware_sdk/build-pico-arm -G Ninja

PICO_SDK_PATH="$PWD/third_party/pico-sdk" \
PICO_TOOLCHAIN_PATH="$PWD/third_party/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi/bin" \
cmake --build firmware_sdk/build-pico-arm -j
```

## Flash
- Reboot to BOOTSEL: `picotool reboot -u -f`
- Copy UF2 to `RPI-RP2`.
