#include "pump.h"
#include <Arduino.h>
#include "config/pin.h"

namespace pump
{
    void initialize()
    {
        pinMode(pin::PUMP_PIN, OUTPUT);
        Serial.println("[ACTUATOR] Initialize pump");
    }
    void on()
    {
        digitalWrite(pin::PUMP_PIN, HIGH);
    }
    void off()
    {
        digitalWrite(pin::PUMP_PIN, LOW);
    }
}