# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP8266/ESP32 firmware for reading water/gas consumption data from Itron EverBlu Cyble Enhanced RF meters using CC1101 433 MHz transceiver. Integrates with Home Assistant via MQTT AutoDiscovery or native ESPHome component.

## Build Commands

```bash
# Build for specific board (default: huzzah)
pio run -e huzzah          # Adafruit HUZZAH ESP8266
pio run -e d1_mini         # WeMos D1 Mini
pio run -e d1_mini_pro     # WeMos D1 Mini Pro
pio run -e nodemcuv2       # NodeMCU v2
pio run -e esp32dev        # ESP32 DevKit

# Upload via USB
pio run -e huzzah -t upload

# Upload and monitor
pio run -e huzzah -t upload -t monitor

# OTA upload (set device IP in platformio.ini first)
pio run -e huzzah-ota -t upload
```

## Testing

```bash
pio test                   # Run all tests
pio test -e huzzah         # Test on specific board
pio test -v                # Verbose output
```

Tests use Unity framework. Test files are in `/test/`:
- `test_utils.cpp` - CRC calculation, hex display
- `test_config_validation.cpp` - Configuration validation
- `test_state_machine.cpp` - State machine logic

## Code Quality

```bash
# Static analysis (matches CI)
cppcheck --enable=all --suppress=missingIncludeSystem --suppress=unusedFunction \
  --std=c++11 -I include src/

# Format check (Google style, 120 column limit)
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -style=Google -dry-run -Werror
```

## Architecture

### Dual-Mode Design

The codebase supports two integration methods sharing 95% of core code:

1. **Standalone MQTT Mode**: Entry point `src/main.cpp`, compile-time config from `include/private.h`
2. **ESPHome Mode**: Component in `ESPHOME/components/everblu_meter/`, YAML configuration

### Adapter Pattern (Dependency Injection)

Platform-specific behavior is abstracted via interfaces in `src/adapters/`:

```
IConfigProvider  → DefineConfigProvider (compile-time) / ESPHomeConfigProvider (YAML)
IDataPublisher   → MQTTDataPublisher / ESPHomeDataPublisher
ITimeProvider    → NTPTimeProvider / ESPHomeTimeProvider
```

### Key Modules

- `src/core/cc1101.cpp` - CC1101 radio driver implementing RADIAN protocol
- `src/core/utils.cpp` - CRC-16/KERMIT, hex encoding, serial decoding
- `src/services/meter_reader.cpp` - Main orchestrator for meter reading
- `src/services/frequency_manager.cpp` - Adaptive frequency tracking
- `src/services/schedule_manager.cpp` - Daily reading schedule
- `src/services/storage_abstraction.cpp` - ESP8266 EEPROM / ESP32 Preferences abstraction

### Frame Validation Pipeline

1. Custom serial decoding (1+8+3 bit framing with 4x oversampling)
2. CRC-16/KERMIT checksum verification
3. Preamble pattern matching (0xAAAAAAAA)
4. Meter serial number validation
5. Signal quality filtering (RSSI/LQI thresholds)

## Configuration

Copy `include/private.example.h` to `include/private.h` and configure:
- WiFi credentials
- MQTT broker settings
- `METER_YEAR` and `METER_SERIAL` (from meter label, omit leading zeros)
- `METER_TYPE` ("water" or "gas")
- `FREQUENCY` (default 433.82 MHz, auto-calibrated)

## ESPHome Component

The ESPHome component in `ESPHOME-release/` is auto-generated from source via `prepare-component-release.sh`. Do not edit `ESPHOME-release/` directly - modify source files and regenerate.

Example configs: `ESPHOME/example-*.yaml`

## Platform Differences

- **ESP8266**: Uses EEPROM for storage, hardware SPI on GPIO 12-15, GDO0 default GPIO 5
- **ESP32**: Uses Preferences API for storage, standard SPI pins, GDO0 default GPIO 4
- **Both**: Arduino framework only (ESP-IDF not supported for this component)
