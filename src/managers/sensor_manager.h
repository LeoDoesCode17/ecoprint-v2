#pragma once

namespace sensor_manager
{
    void initialize();
    float thermocouple_temperature_celcius();
    float sht3x_temperature_celcius();
    float sht3x_humidity_percent();
    long getEncoderPosition();
    void setEncoderPosition(long position);
    bool wasEncoderButtonPressed();
    float smoothed_thermocouple_temperature_celcius(int idx);
    void set_setpoint_temperature(float temperature);
    float get_setpoint_temperature();
    void set_timer(int timer);
    int get_timer();
}