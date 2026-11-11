# Ecoprint v2

ESP32 firmware for **HILIRISET ECOPRINT**, a temperature-controlled natural-dye steaming rig used in the ecoprint process. The device reads water and ambient temperature, holds the water at a user-defined setpoint with a hysteresis controller (servo valve + pump), runs a countdown timer, and streams telemetry to an MQTT broker. Everything is operated on-device through a 3.5" TFT UI driven by a single rotary encoder, and a run can also be started or stopped remotely over MQTT.

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![Build](https://img.shields.io/badge/build-PlatformIO-orange)

> **Project status:** the UI, networking, MQTT interface, state machine, and control loop are implemented. The sensor dashboard is currently running on **simulated sensor values** instead of the real sensor readings (see [Known Limitations](#known-limitations)), so the firmware is not yet ready for an unattended real run.

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
- **Run state machine** (`PREPARATION → FILLING_WATER → FIRING → HEATING → STEAMING → COMPLETED`) that drives the pump, valve, and lighter during a session
- **Smoothed water temperature** support from a MAX6675 thermocouple using an exponential moving average (EMA) filter
- **Ambient monitoring** (air temperature and humidity) with an SHT3x sensor
- **Safety/process inputs** for flame detection (IR) and min/max water level (float switches)
- **On-device UI** on a 3.5" TFT (480x320) navigated with one rotary encoder and its push button, including a landing screen and an on-screen keyboard
- **Configurable steaming session**: setpoint 50-100 °C, timer 1-120 minutes
- **On-device network setup**: WiFi and MQTT credentials are entered on the device and stored in flash (LittleFS), so no re-flashing is needed to change networks
- **MQTT telemetry** of sensor, actuator, and device-status messages as JSON with ISO 8601 UTC timestamps (NTP-synced) and per-topic sequence IDs for packet-loss measurement
- **Remote start/stop** over MQTT with validation, plus `run_started` / `run_finished` / `command_rejected` event messages
- **Dual-core FreeRTOS design**: networking runs on core 0, UI/control and command handling on core 1, connected by queues
- **Built-in service tools**: actuator test page, manual servo control, per-sensor test pages, thermocouple offset calibration

## Hardware

| Component                              | Purpose                           | Interface      |
|----------------------------------------|-----------------------------------|----------------|
| ESP32 DevKit (`esp32dev`)              | Main controller                   | n/a            |
| 3.5" TFT, ILI9488, 320x480             | User interface                    | SPI (TFT_eSPI) |
| Rotary encoder with push button        | Menu navigation and value editing | GPIO           |
| MAX6675 + thermocouple                 | Water temperature                 | Software SPI   |
| SHT3x (address `0x44`)                 | Air temperature and humidity      | I²C            |
| Servo (500-2400 µs pulse range, 50 Hz) | Valve actuation                   | PWM            |
| Pump (via driver/relay)                | Water pump, active HIGH           | GPIO           |
| Electric lighter (via driver/relay)    | Ignition, active HIGH             | GPIO           |
| IR flame sensor                        | Flame detection, active LOW       | GPIO (input)   |
| Float switches (min and max)           | Water level, reads HIGH when wet  | GPIO (input)   |

> If your TFT uses a different controller (ILI9481, ILI9486, etc.), change the `-DILI9488_DRIVER` build flag in `platformio.ini`.

## Pin Mapping

Defined in [`src/config/pin.h`](src/config/pin.h).

| Signal                       | GPIO         |
|------------------------------|--------------|
| Thermocouple CLK / CS / SO   | 27 / 14 / 13 |
| I²C SDA / SCL (SHT3x)        | 21 / 22      |
| Servo valve                  | 25           |
| Rotary encoder CLK / DT / SW | 32 / 33 / 26 |
| Pump                         | 16           |
| Electric lighter             | 17           |
| IR flame sensor (input)      | 34           |
| Max float switch (input)     | 39           |
| Min float switch (input)     | 35           |

TFT pins are set through build flags in `platformio.ini`: MISO 19, MOSI 23, SCLK 18, CS 5, DC 2, RST 4.

> GPIO 34, 35, and 39 are input-only on the ESP32 and have no internal pull-ups, so the flame sensor and float switches need external pull-up resistors.

## Project Structure

```
ecoprint-v2/
├── platformio.ini            # Build environments, libraries, TFT_eSPI flags
└── src/
    ├── main.cpp              # setup() and the four FreeRTOS tasks
    ├── config/
    │   ├── constants.h       # Timing, filter, hysteresis, valve and range constants
    │   ├── pin.h             # GPIO assignments
    │   └── type.h            # Shared structs, command/message types, StateMachine enum
    ├── sensors/              # Low-level drivers: thermocouple, SHT3x, IR flame,
    │                         #   float switches, rotary encoder
    ├── actuators/            # Low-level drivers: servo valve, pump, lighter
    ├── managers/
    │   ├── sensor_manager      # Sensor access, EMA filter, setpoint/timer/offset state
    │   ├── actuator_manager    # Valve/pump/lighter control and status
    │   ├── network_manager     # JSON payloads, topics, NTP time, sequence IDs, credentials
    │   ├── queue_manager       # Outbound-message and command queues between tasks
    │   ├── state_manager       # Device state machine and start/stop command handling
    │   ├── file_system_manager # LittleFS storage of the network config
    │   └── page_manager        # Page navigation and encoder dispatch
    ├── networks/             # WiFi and MQTT (PubSubClient) wrappers
    └── pages/                # UI screens (one class per page, shared IPage interface)
```

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- USB cable and the hardware listed above
- A reachable MQTT broker (plain MQTT, typically port 1883)

### Build and flash

```bash
# Clone the repository
git clone https://github.com/LeoDoesCode17/ecoprint-v2.git
cd ecoprint-v2

# Build
pio run -e esp32dev

# Upload to the board
pio run -e esp32dev -t upload

# Open the serial monitor (115200 baud)
pio device monitor
```

Library dependencies (TFT_eSPI, Adafruit SHT31, MAX6675, ESP32Servo, ESP32Encoder, ESP32RotaryEncoder, PubSubClient, ArduinoJson) are fetched automatically by PlatformIO on the first build. LittleFS ships with the ESP32 Arduino core.

### First boot

1. On the very first boot the flash is blank, so LittleFS is formatted and no network config exists.
2. Open **Setting → Network**, then enter your WiFi SSID/password and MQTT broker host/port (and optionally username/password) using the on-screen keyboard.
3. Open **Network → Status** and choose **Connect**. The device saves the config to flash and restarts to apply it.

On later boots the saved config is loaded automatically.

## Configuration

### Network (on-device)

WiFi and MQTT settings are edited on the device and saved to `/network-config.json` in LittleFS:

| Field               | Notes                                    |
|---------------------|------------------------------------------|
| WiFi SSID           | Required, up to 32 characters            |
| WiFi password       | Up to 63 characters                      |
| MQTT broker host    | Required, up to 63 characters            |
| MQTT port           | Numeric, 1-65535                         |
| MQTT username / password | Optional, up to 63 characters, stored but not yet used when connecting (see [Known Limitations](#known-limitations)) |

Credentials are stored in plain text in flash and are never committed to the repository.

### Build-time constants

Edit [`src/config/constants.h`](src/config/constants.h) before flashing to change firmware behavior:

| Constant                                  | Description                                                       |
|-------------------------------------------|-------------------------------------------------------------------|
| `MIN_SETPOINT_CELCIUS`, `MAX_SETPOINT_CELCIUS` | Allowed setpoint range (default `50`-`100`)                  |
| `MIN_TIMER_MINUTE`, `MAX_TIMER_MINUTE`    | Allowed timer range (default `1`-`120`)                           |
| `EMA_SMOOTHING_FACTOR`                    | Smoothing coefficients for the temperature filter                 |
| `UPPER_HYSTERESIS_BAND`                   | Degrees above setpoint at which the pump turns on (default `5.0`) |
| `LOWER_HYSTERESIS_BAND`                   | Degrees below setpoint at which heating resumes (default `0.5`)   |
| `THERMOCOUPLE_OFFSET`                     | Default thermocouple offset (default `0.0`)                       |
| `VALVE_PERCENT_TO_DEGREE_*`               | Valve percent-to-servo-angle mapping                              |

The thermocouple offset can also be adjusted at runtime from the device menu; it is held in RAM and resets on reboot.

## Using the Device

Navigate by turning the encoder; press the button to select. The device starts on a **HILIRISET ECOPRINT** landing screen; press the button to enter the main menu.

```
Main Menu
├── Start Steaming       Opens the session setup page (same page as Collect Data)
├── Test Actuator        Toggle pump, lighter, and valve (valve opens to 90°)
├── Modify Servo         Set a servo angle (0-90°) and apply it
├── Collect Data         Set temperature (50-100 °C) and timer (1-120 min), then Start
│   └── Sensor Dashboard Live view: time left, set temp, water/air temp, humidity,
│       │                valve, pump and lighter state. Buttons: Lighter toggle, Stop.
│       └── Steaming Summary   Shows the run's setpoint and timer; press to return to the menu
└── Setting
    ├── Network
    │   ├── Wifi             Edit SSID and password
    │   ├── MQTT             Edit broker host, port, username, password
    │   └── Status           Connection state, saved values, Connect (save + restart)
    ├── Sensor Calibration
    │   └── Thermocouple     Apply a +/- offset to the water temperature reading
    ├── Process Calibration
    │   └── Hysteresis       (placeholder screen, not yet implemented)
    └── Test Sensor
        ├── Test Thermocouple
        ├── Test SHT3X
        ├── Test Float Switch
        └── Test IR Flame
```

**Running a session:** open **Start Steaming**, select a slider, press to edit, turn to change, press to confirm, then choose **Start**. The device enters the run state machine and the UI switches to the dashboard. A session ends when the countdown reaches zero, when you choose **Stop**, or when a remote stop command arrives (the UI jumps to the summary). Leaving the dashboard always turns the pump and lighter off and closes the valve.

**Applying network changes:** new WiFi/MQTT credentials only take effect after a restart. The **Connect** button on **Network → Status** asks for confirmation (warning you if a run is in progress), saves the config, puts the actuators in a safe state, and restarts the ESP32.

## Control Logic

While a session is running, the dashboard runs a hysteresis controller every 90 ms against the water temperature `T` and the setpoint `S`:

| Condition                   | Valve       | Pump      |
|-----------------------------|-------------|-----------|
| `T >= S + 5.0` (upper band) | Narrow (9°) | On        |
| `S <= T < S + 5.0`          | Narrow (9°) | unchanged |
| `T <= S - 0.5` (lower band) | Wide (30°)  | Off       |
| Between `S - 0.5` and `S`   | unchanged   | unchanged |

### State machine

The device exposes a `StateMachine` value (published in status messages). A session moves through:

| State           | What happens                                                                 |
|-----------------|------------------------------------------------------------------------------|
| `IDLE`          | Waiting. A start command (local or remote) moves to `PREPARATION`.           |
| `PREPARATION`   | Setpoint and timer are applied; the UI switches to the dashboard.            |
| `FILLING_WATER` | Pump runs until the water level is sufficient.                               |
| `FIRING`        | Valve opens wide and the lighter is turned on until a flame is detected.     |
| `HEATING`       | Heats until the water temperature reaches the setpoint.                      |
| `STEAMING`      | Holds the setpoint with the hysteresis controller until the timer ends.      |
| `COMPLETED`     | Set when a stop command is handled (timer end, Stop button, or remote stop). |

`FAILED` and `ERROR` are defined and cause new start commands to be rejected, but no code path sets them yet.

Sensors are sampled and telemetry is queued roughly every 2 seconds; actuator state is refreshed every 90 ms.

## MQTT Interface

`<MAC>` is the ESP32 Wi-Fi MAC address in the form `AA:BB:CC:DD:EE:FF`, printed on the serial monitor at boot. All timestamps are UTC ISO 8601 with millisecond precision (`recorded_at`), synced over NTP.

### Published topics

| Topic                          | When                                  | Payload fields                                                                                                  |
|--------------------------------|---------------------------------------|-----------------------------------------------------------------------------------------------------------------|
| `esp/<MAC>/telemetry/sensor`   | Every ~2 s while the dashboard is open | `water_temperature`, `air_temperature`, `humidity`, `is_water_sufficient`, `is_fire_on`, `recorded_at`, `seq_id` |
| `esp/<MAC>/telemetry/actuator` | Every ~2 s while the dashboard is open | `valve_degree`, `is_valve_open`, `is_max_valve_opening`, `is_pump_on`, `is_lighter_on`, `recorded_at`, `seq_id` |
| `esp/<MAC>/telemetry/status`   | Every 5 s                             | `state_machine` (integer), `recorded_at`, `seq_id`                                                              |
| `esp/<MAC>/event`              | On accepted/rejected commands         | See below                                                                                                       |

### Subscribed topic

| Topic              | Payload                                                         | Effect                                          |
|--------------------|-----------------------------------------------------------------|-------------------------------------------------|
| `esp/<MAC>/command` | `{"command": 1, "setpoint": 90, "timer_minutes": 60}`          | Start a run (validated, see below)              |
| `esp/<MAC>/command` | `{"command": 0}`                                               | Stop the current run (state becomes `COMPLETED`) |

A start command is rejected if the setpoint is outside 50-100 °C, the timer is outside 1-120 minutes, a run is already in progress, or the device is in `FAILED`/`ERROR`. Local (on-device) commands go through the same validation.

### Event messages

Published on `esp/<MAC>/event`:

```json
{
  "recorded_at": "2026-11-07T08:30:00.120Z",
  "event": "run_started",
  "source": "remote",
  "setpoint": 90,
  "timer_minutes": 60,
  "is_allowed": 1
}
```

`event` is `run_started` or `run_finished` for accepted commands. A rejected remote start is published as:

```json
{
  "recorded_at": "2026-11-07T08:30:05.480Z",
  "event": "command_rejected",
  "setpoint": 90,
  "timer_minutes": 60,
  "reason": "already_running",
  "state_machine": 4,
  "source": "remote",
  "command": 1,
  "is_allowed": 0
}
```

`reason` is one of `already_running`, `device_error`, or `invalid_value`.

### Example sensor message

```json
{
  "water_temperature": 87.4,
  "air_temperature": 29.1,
  "humidity": 71.0,
  "is_water_sufficient": true,
  "recorded_at": "2026-11-07T14:21:05.482Z",
  "is_fire_on": true,
  "seq_id": 42
}
```

`seq_id` increments separately for the sensor, actuator, and status message types, and resets when you leave the summary page. Comparing the last received `seq_id` with the number of messages a subscriber actually received is how packet loss is measured.

## Architecture Notes

- **Layering:** drivers (`sensors/`, `actuators/`) → managers (`managers/`) → pages and `main.cpp`. Pages never touch hardware directly; they go through managers.
- **FreeRTOS tasks** (created in `setup()`; `loop()` deletes itself):
  - `NetworkTask` (core 0): mounts LittleFS, initializes WiFi/MQTT, reconnects, runs the MQTT loop, and publishes messages taken from the outbound queue.
  - `MainTask` (core 1): runs `page_manager::update()` every 10 ms (UI, encoder, and the control loop on the dashboard).
  - `PublishDeviceStatusTask` (core 1): queues a device-status message every 5 s.
  - `CommandReactTask` (core 1): blocks on the command queue and applies start/stop commands through `state_manager`.
- **Queues:** `queue_manager` provides an outbound message queue (length 10) and a command queue (length 5), so UI/control code never calls MQTT directly. Local commands (UI) and remote commands (MQTT) share the same command queue.
- **State-driven navigation:** when the state machine changes, `page_manager` automatically moves to the dashboard (running states) or the summary (`COMPLETED`/`FAILED`/`ERROR`), so a remote start or stop is reflected on the screen.
- **Modules are namespaces**, not classes, except for UI pages, which implement the `IPage` interface (`onEnter`, `onExit`, `update`). Credential screens share a `CredentialPage` base and an on-screen keyboard.
- **Adding a page:** add a value to `PageId` (`pages/page_id.h`), create the page class implementing `IPage`, and add one `case` in `pageFor()` in `page_manager.cpp`.
- **Encoder handling:** `page_manager` reads the encoder once per loop, passes the delta and button press to the active page, and zeroes the counter on every navigation.
- **PlatformIO environments:** `esp32dev` is the primary target. An `esp32-s3-devkitc-1` environment is also defined with the same libraries and build flags; the GPIO map in `pin.h` is written for the classic ESP32 DevKit, so review it before using the S3.

## Known Limitations

- **The sensor dashboard uses simulated values.** In `src/pages/sensor_dashboard_page.cpp`, water temperature (87.5 °C), air temperature, humidity, flame detected, and water sufficient are hardcoded, and the real `sensor_manager` readings are commented out. The state machine and control loop therefore run against fake data until those lines are restored.
- `is_fire_on` and `is_water_sufficient` in the outbound sensor message are currently assigned from the water temperature value, so they always publish `true`.
- MQTT username/password can be configured and saved, but are not yet passed to the broker when connecting, and there is no TLS. The MQTT client ID is a fixed string (`ESP32`).
- WiFi and MQTT connection loops are blocking (with no timeout) and the NTP sync can block for several seconds. They run on core 0, so the UI stays responsive, but the device keeps retrying forever if the credentials are wrong.
- Actuator messages are only published on the ~2 s timer; publishing immediately on a state change is not enabled.
- `constants.h` still contains older leftovers (placeholder WiFi values, example topics including a hardcoded MAC, and mDNS/Raspberry Pi settings). The MQTT layer still subscribes to some of these legacy topics on connect.
- The Hysteresis calibration screen is a placeholder.
- The thermocouple offset is not persisted across reboots; only the network config is stored in flash.
- `FAILED` and `ERROR` states are not yet triggered by any fault detection.
- Network credentials are stored unencrypted in LittleFS.
- The demo pages (`Page1`-`Page3`) remain in the code but are not reachable from the menu.

## License

This project is licensed under the MIT License. See [LICENSE.md](LICENSE.md) for details.