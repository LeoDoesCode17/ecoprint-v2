#include "float_switch.h"
#include "config/pin.h"

namespace float_switch
{
    void initialize()
    {
        Serial.println("[SENSOR] Initalize max float and min float switch");
        pinMode(pin::MAX_FLOAT_SWITCH_PIN, INPUT);
        pinMode(pin::MIN_FLOAT_SWITCH_PIN, INPUT);
    }
    int get_max_float_switch_state()
    {
        return digitalRead(pin::MAX_FLOAT_SWITCH_PIN);
    }
    int get_min_float_switch_state()
    {
        return digitalRead(pin::MIN_FLOAT_SWITCH_PIN);
    }
}