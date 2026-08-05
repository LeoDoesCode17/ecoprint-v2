#pragma once
#include <stdint.h>
using mqtt_callback = void (*)(char *, uint8_t *, unsigned int);

enum class EcoprintEvent : uint8_t
{
    PREPARATION = 0,
    STEAMING = 1,
};

typedef struct
{
    float water_temperature;
    float air_temperature;
    float air_humidity;
    bool is_water_sufficient;
    EcoprintEvent event;
} ecoprint_sensor_t;

typedef struct
{
    int state_machine;
    bool is_active;
} ecoprint_device_t;

enum state_machine
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