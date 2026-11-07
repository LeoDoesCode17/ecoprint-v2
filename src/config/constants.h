#pragma once
#include <Arduino.h>
namespace constant
{
    const unsigned int ZERO_PERCENT_DEGREE = 0;
    // const unsigned int ZERO_PERCENT_DEGREE = 9;
    const unsigned int VALVE_PERCENT_TO_DEGREE_GRADIENT = 94;
    const unsigned int VALVE_PERCENT_TO_DEGREE_OFFSET = 6;
    const uint8_t MIN_SETPOINT_CELCIUS = 50;
    const uint8_t MAX_SETPOINT_CELCIUS = 100;
    const uint8_t MIN_TIMER_MINUTE = 1;
    const uint8_t MAX_TIMER_MINUTE = 120;

    const unsigned long MAIN_LOOP_INTERVAL_MS = 10;
    const unsigned long PUBLISH_SENSOR_DATA_INTERVAL_MS = 1990;
    const unsigned long UPDATE_SENSOR_DATA_INTERVAL_MS = 1990;
    const unsigned long PUBLISH_DEVICE_STATUS_INTERVAL_MS = 10000;

    const int EMA_SMOOTHING_FACTOR_SIZE = 5;
    const float EMA_SMOOTHING_FACTOR[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};

    const float UPPER_HYSTERESIS_BAND = 5.0f;//0.559958f;//1.0f;
    const float LOWER_HYSTERESIS_BAND = 0.5f; //0.330914f;//1.0f;
    const float THERMOCOUPLE_OFFSET = 0.0f;//-5.4f;

    const int SHT3X_ADDR = 0x44;

    const long INVALID_SEQ_ID = -999;
}