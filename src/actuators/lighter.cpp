#include "lighter.h"
#include <Arduino.h>
#include "config/pin.h"

namespace lighter
{
    void initialize()
    {
        pinMode(pin::ELECTRICAL_LIGHTER_PIN, OUTPUT);
        Serial.println("[ACTUATOR] Lighter initialize");
    }
    void on()
    {
        digitalWrite(pin::ELECTRICAL_LIGHTER_PIN, HIGH);
    }
    void off()
    {
        digitalWrite(pin::ELECTRICAL_LIGHTER_PIN, LOW);
    }
}