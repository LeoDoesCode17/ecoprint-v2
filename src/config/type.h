#pragma once
#include <Arduino.h>

using mqtt_callback = void (*)(char *, uint8_t *, unsigned int);

enum class EcoprintEvent : uint8_t
{
    PREPARATION = 0,
    STEAMING = 1,
};

enum class command_source_t : uint8_t
{
    LOCAL,
    REMOTE
};

enum class reject_reason_t : uint8_t
{
    ALREADY_RUNNING,
    DEVICE_ERROR,
    INVALID_VALUE,
};

typedef struct ecoprint_sensor_t
{
    float water_temperature;
    float air_temperature;
    float humidity;
    bool is_water_sufficient;
    bool is_fire_on;
    int setpoint;
} ecoprint_sensor_t;

typedef struct ecoprint_actuator_t
{
    int valve_degree;
    bool is_valve_open;
    bool is_max_valve_opening;
    bool is_pump_on;
    bool is_lighter_on;
    int setpoint;
} ecoprint_actuator_t;

typedef struct
{
    float water_temperature;
    float smoothing_factor;
} ema_filter_sensor_data_t;

enum class StateMachine : uint8_t
{
    IDLE,
    PREPARATION,
    FILLING_WATER,
    FIRING,
    HEATING,
    STEAMING,
    COMPLETED,
    FAILED,
    ERROR
};

typedef struct
{
    StateMachine state_machine;
    bool is_active;
} ecoprint_device_t;

typedef struct
{
    int command;
    int setpoint;
    int timer_minutes;
    command_source_t source;
} command_t;

typedef struct
{
    int command;
} ecoprint_command_t;

typedef struct
{
    float water_temperature;
    float air_temperature;
    float humidity;
    bool is_water_sufficient;
    bool is_fire_on;
} sensor_t;

typedef struct
{
    int valve_degree;
    bool is_valve_open;
    bool is_max_valve_opening;
    bool is_pump_on;
    bool is_lighter_on;
} actuator_t;

typedef struct
{
    command_t command; // the command that was rejected
    reject_reason_t reason;
    StateMachine state; // device state machine at the rejection time
} command_rejected_t;

typedef struct
{
    StateMachine state_machine;
} device_status_t;

typedef struct
{
    int setpoint;
    int steaming_duration_minutes;
} ecoprint_start_command_t;

enum class msg_type_t : uint8_t
{
    SENSOR,
    ACTUATOR,
    DEVICE_STATUS,
    COMMAND_EVENT,
    COMMAND_REJECTED,
};

typedef struct
{
    msg_type_t type;
    union
    {
        sensor_t sensor;
        actuator_t actuator;
        device_status_t device_status;
        command_t command_event;
        command_rejected_t command_rejected;
    };
} outbound_msg_t;