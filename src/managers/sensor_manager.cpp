#include "sensor_manager.h"
#include "sensors/thermocouple.h"
#include "sensors/sht3x.h"

namespace sensor_manager
{
    void initialize()
    {
        sht3x::initialize();
    }
    float sht3x_temperature_celcius()
    {
        return sht3x::temprature_celcius();
    }
    float sht3x_humidity_percent()
    {
        return sht3x::humidity_percent();
    }
    float thermocouple_temperature_celcius()
    {
        return thermocouple::temperature_celcius();
    }
}