#pragma once
#include <stdint.h>
using mqtt_callback = void (*)(char *, uint8_t *, unsigned int);

enum class EcoprintEvent : uint8_t
{
    PREPARATION = 0,
    STEAMING = 1,
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