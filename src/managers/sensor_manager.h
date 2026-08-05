#pragma once

namespace sensor_manager {
    void initialize();
    float thermocouple_temperature_celcius();
    float sht3x_temperature_celcius();
    float sht3x_humidity_percent();
}