#include "rotary_encoder.h"
#include <ESP32Encoder.h>

namespace
{
    ESP32Encoder encoder;
    uint8_t buttonPin = 0;

    const unsigned long DEBOUNCE_MS = 30;
    bool lastRawState = HIGH;
    bool stableState = HIGH;
    unsigned long lastDebounceTime = 0;
}

namespace rotary_encoder
{
    void initialize(uint8_t dtPin, uint8_t clkPin, uint8_t swPin)
    {
        ESP32Encoder::useInternalWeakPullResistors = puType::up;
        encoder.attachHalfQuad(dtPin, clkPin);
        encoder.setCount(0);

        buttonPin = swPin;
        pinMode(buttonPin, INPUT_PULLUP);
    }

    long getPosition()
    {
        return static_cast<long>(encoder.getCount() / 2);
    }

    void setPosition(long position)
    {
        encoder.setCount(position * 2);
    }

    bool wasButtonPressed()
    {
        bool rawState = digitalRead(buttonPin);
        bool pressedEdge = false;

        if (rawState != lastRawState)
        {
            lastDebounceTime = millis();
        }

        if ((millis() - lastDebounceTime) > DEBOUNCE_MS && rawState != stableState)
        {
            stableState = rawState;
            if (stableState == LOW)
            {
                pressedEdge = true;
            }
        }

        lastRawState = rawState;
        return pressedEdge;
    }
}