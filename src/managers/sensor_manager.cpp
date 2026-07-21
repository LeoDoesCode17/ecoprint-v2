#include "sensor_manager.h"
#include "sensors/thermocouple.h"

namespace sensor_manager
{
    float thermocouple_temperature_celcius()
    {
        return thermocouple::temperature_celcius();
    }
}