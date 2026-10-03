#include "ir_flame.h"
#include "config/pin.h"

namespace ir_flame
{
    void intialize()
    {
        Serial.println("[SENSOR] Initialize IR Flame with external pull up resistor");
        pinMode(pin::IR_FLAME_OUTPUT_PIN, INPUT); //
    }
    int get_digital_output()
    {
        return digitalRead(pin::IR_FLAME_OUTPUT_PIN);
    }
}