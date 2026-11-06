# ESP32 Lab Studio

A Linux desktop workspace for compiling Arduino `.ino` sketches for an ESP32-S3 and running the resulting firmware in Espressif's ESP-EMU.

**This is an integration frontend around Espressif's emulator, not a new CPU emulator.** It uses the upstream emulator for firmware execution, Wi-Fi, CPU, memory and supported MCU peripherals.

## Target

- Chip: **ESP32-S3**
- Arduino board profile: `esp32:esp32:esp32s3` (ESP32-S3 Dev Module)
- Host: Linux x86_64 (developed for Arch Linux)
- GUI: C++17 / Qt 6 Widgets
- Firmware: Arduino `.ino`

## Features in this starter release

- Select an Arduino `.ino` file.
- Compile it with Arduino CLI and the Espressif Arduino core.
- Merge bootloader, partition table and application into a flash image.
- Launch native `esp-emu` with an emulated Wi-Fi access point.
- Capture emulator UART output in the desktop console.
- Save a component/wiring plan for LEDs, relays, buttons, toggle switches, buzzers, sensors and RTC modules.
- Open the upstream browser GPIO dashboard to observe outputs and drive input pins.

## Important simulation boundary

The component table is currently a **wiring-plan editor**, not a circuit simulator. ESP-EMU runs the ESP32 firmware and models supported SoC peripherals. Its browser dashboard lets you click GPIO input pads and observe GPIO output pads. This starter does **not** yet electrically couple the custom relay/LED/switch objects to the native emulator process, and it does not model relay contacts, analog circuits, switch bounce, current draw or mains loads.

For an interactive firmware test today:
1. Run the firmware in the native emulator from this app.
2. Open the GPIO dashboard.
3. Load the same `build/firmware/merged_flash.bin` into the browser dashboard.
4. Click GPIO input pads to drive them high/low; observe output pads and UART logs.

The browser dashboard is provided by the ESP-EMU project. Networking in the hosted dashboard has limitations; use the native emulator for the built-in user-mode network backend.

## Install dependencies (Arch Linux)

```bash
sudo pacman -S --needed base-devel cmake qt6-base arduino-cli python python-pipx
```

Install ESP32 Arduino platform:

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

Install esptool:

```bash
pipx install esptool
```

Install Espressif ESP-EMU:

```bash
bash scripts/install_esp_emu.sh
```

Make sure `~/.local/bin` is on PATH:

```bash
export PATH="$HOME/.local/bin:$PATH"
```

## Build the desktop app

From the project root:

```bash
cmake -S . -B build/app
cmake --build build/app -j"$(nproc)"
```

Run:

```bash
./build/app/esp32-lab-studio
```

Launch it from the project root so relative scripts and paths resolve correctly.

## Compile and run without the GUI

```bash
bash scripts/build_firmware.sh
bash scripts/run_emulator.sh
```

Or select a different sketch:

```bash
bash scripts/build_firmware.sh /absolute/path/to/YourSketch/YourSketch.ino
```

Ensure `~/.local/bin` is on `PATH` so the `esptool` command installed by pipx can be found.

The compiled merged image is written to:

```text
build/firmware/merged_flash.bin
```

## Wi-Fi behavior

The native ESP-EMU backend provides an emulated Wi-Fi access point. The sketch SSID/password must match the SSID/password entered in the app. The default demo sketch uses `BellLab` / `belllab123`. These are local test credentials, not real network credentials.

ESP-EMU's user-mode network backend provides a virtual network path. It is not identical to every real-world Wi-Fi router, and services depending on multicast, broadcast, captive portals or LAN discovery may behave differently.

## Component wiring plan

The default `bell-lab.json` maps:
- Relay module → GPIO5, Active LOW
- Push button → GPIO4
- Status LED → GPIO2

These are example assignments only. Check the exact pinout of your ESP32-S3 board and avoid boot-strapping, flash/PSRAM, USB or input-only pins where applicable. Never connect a relay coil or mains load directly to an ESP32 GPIO. Use a suitable driver/isolated relay module and qualified mains wiring.

## Project structure

```text
esp32-lab-studio/
├── CMakeLists.txt
├── README.md
├── bell-lab.json
├── src/
│   ├── main.cpp
│   ├── MainWindow.cpp
│   └── MainWindow.hpp
├── scripts/
│   ├── build_firmware.sh
│   ├── install_esp_emu.sh
│   └── run_emulator.sh
├── examples/
│   └── BellDemo/
│       └── BellDemo.ino
├── components/
└── docs/
    └── BUILDING.md
```

## Current maturity

This is a runnable first integration project, not a production-certified emulator. ESP-EMU is an upstream beta project. Validate each peripheral and network feature against the specific firmware and chip revision before relying on results for hardware deployment.
