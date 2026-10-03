# Ecoprint v2

ESP32 firmware for a temperature-controlled steaming rig used in the ecoprint process. The device reads water and ambient temperature, holds the water at a user-defined setpoint with a hysteresis controller (servo valve + pump), runs a countdown timer, and streams telemetry to an MQTT broker. Everything is operated on-device through a TFT touch-free UI driven by a single rotary encoder.

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![Build](https://img.shields.io/badge/build-PlatformIO-orange)

## Table of Contents

- [Features](#features)
- [Hardware](#hardware)
- [Pin Mapping](#pin-mapping)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
- [Configuration](#configuration)
- [Using the Device](#using-the-device)
- [Control Logic](#control-logic)
- [MQTT Interface](#mqtt-interface)
- [Architecture Notes](#architecture-notes)
- [Known Limitations](#known-limitations)
- [License](#license)

## Features

- **Closed-loop temperature control** with an upper/lower hysteresis band around a user-set target temperature
- **Smoothed water temperature** from a MAX6675 thermocouple using an exponential moving average (EMA) filter
- **Ambient monitoring** (air temperature and humidity) with an SHT3x sensor
- **Safety/process inputs** for flame detection (IR) and min/max water level (float switches)
- **On-device UI** on a 3.5" TFT (480x320) navigated with one rotary encoder and its push button
- **Configurable steaming session**: setpoint 50-100 °C, timer 30-120 minutes
- **MQTT telemetry** of sensor, actuator, and device-status messages as JSON with ISO 8601 UTC timestamps (NTP-synced) and per-topic sequence IDs for packet-loss measurement
- **Remote commands** over MQTT (start/idle) and an automatic stop message when the timer ends
- **Built-in service tools**: actuator test page, manual servo control, thermocouple offset calibration

## Hardware

| Component | Purpose | Interface |
|---|---|---|
| ESP32 DevKit (`esp32dev`) | Main controller | n/a |
| 3.5" TFT, ILI9488, 320x480 | User interface | SPI (TFT_eSPI) |
| Rotary encoder with push button | Menu navigation and value editing | GPIO |
| MAX6675 + thermocouple | Water temperature | Software SPI |
| SHT3x (address `0x44`) | Air temperature and humidity | I²C |
| Servo (500-2400 µs pulse range, 50 Hz) | Valve actuation | PWM |
| Pump (via driver/relay) | Water pump, active HIGH | GPIO |
| Electric lighter (via driver/relay) | Ignition, active HIGH | GPIO |
| IR flame sensor | Flame detection, active LOW | GPIO (input) |
| Float switches (min and max) | Water level, active LOW | GPIO (input) |

> If your TFT uses a different controller (ILI9481, ILI9486, etc.), change the `-DILI9488_DRIVER` build flag in `platformio.ini`.

## Pin Mapping

Defined in [`src/config/pin.h`](src/config/pin.h).

| Signal | GPIO |
|---|---|
| Thermocouple CLK / CS / SO | 27 / 14 / 13 |
| I²C SDA / SCL (SHT3x) | 21 / 22 |
| Servo valve | 25 |
| Rotary encoder CLK / DT / SW | 32 / 33 / 26 |
| Pump | 16 |
| Electric lighter | 17 |
| IR flame sensor (input) | 34 |
| Max float switch (input) | 39 |
| Min float switch (input) | 35 |

TFT pins are set through build flags in `platformio.ini`: MISO 19, MOSI 23, SCLK 18, CS 5, DC 2, RST 4.

> GPIO 34, 35, and 39 are input-only on the ESP32 and have no internal pull-ups, so the flame sensor and float switches need external pull-up resistors.

## Project Structure

```
ecoprint-v2/
├── platformio.ini            # Build environments, libraries, TFT_eSPI flags
└── src/
    ├── main.cpp              # setup() and the main loop
    ├── config/
    │   ├── constants.h       # WiFi/MQTT settings, timing, filter and control constants
    │   ├── pin.h             # GPIO assignments
    │   └── type.h            # Shared structs and the StateMachine enum
    ├── sensors/              # Low-level drivers: thermocouple, SHT3x, IR flame,
    │                         #   float switches, rotary encoder
    ├── actuators/            # Low-level drivers: servo valve, pump, lighter
    ├── managers/
    │   ├── sensor_manager    # Sensor access, EMA filter, setpoint/timer/offset state
    │   ├── actuator_manager  # Valve/pump/lighter control and status
    │   ├── network_manager   # JSON payloads, topics, NTP time, sequence IDs
    │   ├── page_manager      # Page navigation and encoder dispatch
    │   └── state_manager     # Device state machine value
    ├── networks/             # WiFi and MQTT (PubSubClient) wrappers
    └── pages/                # UI screens (one class per page, shared IPage interface)
```

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- USB cable and the hardware listed above
- A reachable MQTT broker (default port 1883)

### Build and flash

```bash
# Clone the repository
git clone <your-repo-url>
cd ecoprint-v2

# Build
pio run -e esp32dev

# Upload to the board
pio run -e esp32dev -t upload

# Open the serial monitor (115200 baud)
pio device monitor
```

Library dependencies (TFT_eSPI, Adafruit SHT31, MAX6675, ESP32Servo, ESP32Encoder, ESP32RotaryEncoder, PubSubClient, ArduinoJson) are fetched automatically by PlatformIO on the first build.

## Configuration

Edit [`src/config/constants.h`](src/config/constants.h) before flashing:

| Constant | Description |
|---|---|
| `WIFI_SSID`, `WIFI_PASSWORD` | WiFi credentials |
| `ECOPRINT_MQTT_HOST` | MQTT broker hostname |
| `MQTT_PORT` | MQTT broker port (default `1883`) |
| `EMA_SMOOTHING_FACTOR` | Smoothing coefficients for the temperature filter |
| `UPPER_HYSTERESIS_BAND` | Degrees above setpoint at which the pump turns on (default `5.0`) |
| `LOWER_HYSTERESIS_BAND` | Degrees below setpoint at which heating resumes (default `0.5`) |
| `VALVE_PERCENT_TO_DEGREE_*` | Valve percent-to-servo-angle mapping |

> **Do not commit real WiFi credentials.** Keep the placeholders in the repository, or move them to an untracked header (`secrets.h` is already listed in `.gitignore`).

The thermocouple offset can also be adjusted at runtime from the device menu (see below); it is held in RAM and resets on reboot.

## Using the Device

Navigate by turning the encoder; press the button to select.

```
Menu
├── Test Actuator        Toggle pump, lighter, and valve (valve opens to 90°)
├── Modify Servo         Set a servo angle (0-90°) and apply it
├── Collect Data         Set temperature (50-100 °C) and timer (30-120 min), then Start
│   └── Sensor Dashboard Live view: time left, set temp, water/air temp, humidity,
│       │                valve and pump state. Runs the control loop.
│       └── Steaming Summary   Message counters; press to return to the menu
└── Setting
    ├── Sensor Calibration
    │   └── Thermocouple     Apply a +/- offset to the water temperature reading
    └── Process Calibration
        └── Hysteresis       (placeholder screen, not yet implemented)
```

**Running a session:** open **Collect Data**, select a slider, press to edit, turn to change, press to confirm, then choose **Start**. The dashboard begins controlling the valve and pump and publishing telemetry. A session ends when the countdown reaches zero (a stop message is published to MQTT) or when you press the button (jumps to the summary). Leaving the dashboard always turns the pump off and closes the valve.

## Control Logic

The dashboard runs a hysteresis controller every 90 ms against the EMA-filtered water temperature `T` and the setpoint `S`:

| Condition | Valve | Pump |
|---|---|---|
| `T >= S + 5.0` (upper band) | Narrow (9°) | On |
| `S <= T < S + 5.0` | Narrow (9°) | unchanged |
| `T <= S - 0.5` (lower band) | Wide (30°) | Off |
| Between `S - 0.5` and `S` | unchanged | unchanged |

Sensors are sampled and telemetry published roughly every 2 seconds. Actuator state is checked every 90 ms and published immediately on change.

### State machine

The device exposes a `StateMachine` value in its status messages:

`IDLE → PREPARATION → FILLING_WATER → FIRING → HEATING → STEAMING → COMPLETED` (plus `FAILED` and `ERROR`).

Currently the firmware sets `IDLE` and `PREPARATION` (in response to MQTT commands); the remaining states are defined for the upcoming automated sequence.

## MQTT Interface

`<MAC>` is the ESP32 Wi-Fi MAC address in the form `AA:BB:CC:DD:EE:FF`, printed on the serial monitor at boot. All timestamps are UTC ISO 8601 with millisecond precision (`recorded_at`).

### Published topics

| Topic | When | Payload fields |
|---|---|---|
| `esp/<MAC>/telemetry/sensor` | Every ~2 s during a session | `water_temperature`, `air_temperature`, `humidity`, `is_water_sufficient`, `is_fire_on`, `setpoint`, `recorded_at`, `seq_id` |
| `esp/<MAC>/telemetry/actuator` | On change and every ~2 s | `valve_degree`, `is_valve_open`, `is_max_valve_opening`, `is_pump_on`, `is_lighter_on`, `setpoint`, `recorded_at`, `seq_id` |
| `esp/<MAC>/command/stop` | When the timer ends | `command` (`1`), `recorded_at` |
| `esp/<MAC>/ema/telemetry` | Available for filter analysis | `water_temperature`, `smoothing_factor`, `recorded_at` |
| `ecoprint/mac-address/status` | Every 10 s | `is_active`, `state_machine` (integer), `recorded_at`, `seq_id` |

### Subscribed topics

| Topic | Payload | Effect |
|---|---|---|
| `esp/<MAC>/command/start` | `{"command": 0}` | Set state to `IDLE` |
| `esp/<MAC>/command/start` | `{"command": 1}` | Set state to `PREPARATION` |

### Example sensor message

```json
{
  "water_temperature": 87.4,
  "air_temperature": 29.1,
  "humidity": 71.0,
  "is_water_sufficient": true,
  "recorded_at": "2026-10-03T14:21:05.482Z",
  "setpoint": 90,
  "is_fire_on": true,
  "seq_id": 42
}
```

`seq_id` increments per message type. Comparing the last received `seq_id` with the count shown on the **Steaming Summary** screen is how packet loss is measured.

## Architecture Notes

- **Layering:** drivers (`sensors/`, `actuators/`) → managers (`managers/`) → pages and `main.cpp`. Pages never touch hardware directly; they go through managers.
- **Modules are namespaces**, not classes, except for UI pages, which implement the `IPage` interface (`onEnter`, `onExit`, `update`).
- **Adding a page:** add a value to `PageId` (`pages/page_id.h`), create the page class implementing `IPage`, and add one `case` in `pageFor()` in `page_manager.cpp`.
- **Encoder handling:** `page_manager` reads the encoder once per loop, passes the delta and button press to the active page, and zeroes the counter on every navigation.
- **Timing:** the main loop ticks every 10 ms and uses `millis()`-based scheduling rather than blocking delays (apart from WiFi/MQTT reconnection, see below).
- **PlatformIO environments:** `esp32dev` is the primary target. An `esp32-s3-devkitc-1` environment is also defined.

## Known Limitations

- The `esp32-s3-devkitc-1` environment's `lib_deps` is missing PubSubClient, ArduinoJson, and ESP32Encoder, so it needs those added before it will build.
- WiFi and MQTT reconnection are blocking, so the UI freezes while the connection is down.
- The status topic and some topic constants in `constants.h` are static placeholders and are not yet built from the device MAC like the telemetry topics.
- `is_fire_on` and `is_water_sufficient` are currently reported as `true` on the dashboard; the flame and float-switch readings exist in `sensor_manager` but are not yet wired into the published data or control logic.
- The Hysteresis calibration screen is a placeholder.
- The thermocouple offset is not persisted across reboots.
- MQTT connects without TLS or authentication.

## License

No license has been specified yet. Add a `LICENSE` file (for example MIT) before publishing.