#include "thermocouple.h"
#include "max6675.h"
#include "config/pin.h"

namespace
{
    static MAX6675 _thermocouple(pin::THERMO_CLK_PIN, pin::THERMO_CS_PIN, pin::THERMO_D0_PIN);
}
namespace thermocouple
{
    float temperature_celcius()
    {
        return _thermocouple.readCelsius();
    }
}
