# Firmware build notes

ESP-EMU consumes a merged flash image (`.bin`), not a source `.ino` file. The `.ino` is compiled for the selected ESP32-S3 Arduino board profile first.

The helper script:
1. Invokes `arduino-cli compile`.
2. Looks for the Arduino-ESP32 core's generated `flash_args` file.
3. Uses esptool's `merge_bin` command to generate `merged_flash.bin`.
4. Falls back to the common ESP32-S3 Arduino flash offsets only when the required bootloader, partition and boot_app0 artifacts are present.

Arduino-ESP32 build output conventions can change between core releases. If the script cannot find a flash layout, inspect `build/arduino/` and use the generated flash arguments from your installed core version. Do not guess offsets for a custom partition layout.

## Board profile

Default FQBN: `esp32:esp32:esp32s3`

Override it for another supported ESP32-S3 Arduino board definition:

```bash
ESP32_FQBN=esp32:esp32:esp32s3 scripts/build_firmware.sh path/to/sketch.ino
```

The current GUI is intentionally fixed to ESP32-S3. Support for additional chips/boards should be added only after validating their Arduino board profile and ESP-EMU chip target.
