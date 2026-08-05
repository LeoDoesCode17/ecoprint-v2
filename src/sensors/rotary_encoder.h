#pragma once
#include <Arduino.h>

namespace rotary_encoder
{
    void initialize(uint8_t dtPin, uint8_t clkPin, uint8_t swPin);
    long getPosition();
    void setPosition(long position);
    bool wasButtonPressed();
}