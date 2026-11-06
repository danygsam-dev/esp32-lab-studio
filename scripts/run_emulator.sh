#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FIRMWARE="${1:-$ROOT/build/firmware/merged_flash.bin}"
SSID="${WIFI_SSID:-BellLab}"
PASSWORD="${WIFI_PASSWORD:-belllab123}"

command -v esp-emu >/dev/null || { echo "esp-emu not found; run scripts/install_esp_emu.sh"; exit 2; }
[[ -s "$FIRMWARE" ]] || { echo "Firmware image missing: $FIRMWARE"; echo "Compile first with scripts/build_firmware.sh"; exit 2; }

exec esp-emu --chip esp32s3 --firmware "$FIRMWARE" \
  --wifi-ssid "$SSID" --wifi-password "$PASSWORD" --net user
