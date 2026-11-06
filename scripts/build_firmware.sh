#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SKETCH="${1:-$ROOT/examples/BellDemo/BellDemo.ino}"
BUILD="$ROOT/build/arduino"
OUT="$ROOT/build/firmware"
FQBN="${ESP32_FQBN:-esp32:esp32:esp32s3}"

if [[ ! -f "$SKETCH" ]]; then
  echo "ERROR: sketch not found: $SKETCH" >&2
  exit 2
fi
command -v arduino-cli >/dev/null || { echo "ERROR: arduino-cli is not installed." >&2; exit 2; }
command -v esptool >/dev/null || command -v esptool.py >/dev/null || {
  echo "ERROR: esptool is not installed or not on PATH." >&2
  echo "Install it with your distro package manager or pipx." >&2
  exit 2
}
ESPTOOL="$(command -v esptool || command -v esptool.py)"

mkdir -p "$BUILD" "$OUT"
echo "[1/3] Compiling $SKETCH for $FQBN"
arduino-cli compile --fqbn "$FQBN" --build-path "$BUILD" "$SKETCH"

echo "[2/3] Locating ESP32 flash images"
# Arduino-ESP32 core versions commonly emit a flash_args file with the exact
# offsets. Prefer it over hard-coded offsets.
FLASH_ARGS="$(find "$BUILD" -maxdepth 3 -type f \( -name 'flash_args' -o -name 'flash_args.txt' \) -print -quit || true)"
MERGED="$OUT/merged_flash.bin"

if [[ -n "$FLASH_ARGS" ]]; then
  echo "Using flash layout: $FLASH_ARGS"
  (
    cd "$(dirname "$FLASH_ARGS")"
    "$ESPTOOL" --chip esp32s3 merge_bin -o "$MERGED" @"$(basename "$FLASH_ARGS")" \
      || "$ESPTOOL" --chip esp32s3 merge-bin -o "$MERGED" @"$(basename "$FLASH_ARGS")"
  )
else
  echo "No flash_args file found; trying the standard Arduino-ESP32 S3 image layout."
  BOOT="$(find "$BUILD" -type f -name 'bootloader.bin' -print -quit)"
  PART="$(find "$BUILD" -type f -name 'partitions.bin' -print -quit)"
  APP="$(find "$BUILD" -type f -name '*.ino.bin' -o -name 'firmware.bin' | head -n1 || true)"
  BOOTAPP="$(find "$BUILD" -type f -name 'boot_app0.bin' -print -quit)"
  [[ -n "$BOOT" && -n "$PART" && -n "$APP" ]] || {
    echo "ERROR: could not locate bootloader.bin, partitions.bin and application .bin." >&2
    echo "Inspect $BUILD and consult docs/BUILDING.md for your Arduino-ESP32 core version." >&2
    exit 3
  }
  [[ -n "$BOOTAPP" ]] || {
    echo "ERROR: boot_app0.bin missing; cannot safely construct merged image." >&2
    exit 3
  }
  "$ESPTOOL" --chip esp32s3 merge_bin -o "$MERGED" \
    0x0 "$BOOT" 0x8000 "$PART" 0xe000 "$BOOTAPP" 0x10000 "$APP" \
    || "$ESPTOOL" --chip esp32s3 merge-bin -o "$MERGED" \
    0x0 "$BOOT" 0x8000 "$PART" 0xe000 "$BOOTAPP" 0x10000 "$APP"
fi

[[ -s "$MERGED" ]] || { echo "ERROR: merged firmware image was not created." >&2; exit 4; }
echo "[3/3] Firmware ready: $MERGED"
echo "You can now run: esp-emu --chip esp32s3 --firmware \"$MERGED\""
