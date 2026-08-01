#pragma once
#include <stdint.h>
using mqtt_callback = void (*)(char *, uint8_t *, unsigned int);
typedef struct
{
    float water_temperature;
    float air_temperature;
    float air_humidity;
    bool is_water_sufficient;
} ecoprint_sensor_t;
typedef struct
{
    int state_machine;
    bool is_active;
} ecoprint_device_t;
enum state_machine
{
    IDLE,
    FILLING_WATER,
    FIRING,
    HEATING,
    STEAMING,
    COMPLETED,
    FAILED,
    ERROR
};