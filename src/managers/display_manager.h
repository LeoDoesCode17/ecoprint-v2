#pragma once
#include <Arduino.h>

namespace display_manager
{
    using ActionCallback = void (*)();

    // Wire up encoder pins + TFT, draw the initial menu
    void initialize();

    // Call every loop() iteration — handles navigation, click, redraw
    void update();

    // Optional: register/override callbacks after initialize()
    void setActionCallback(uint8_t buttonIndex, ActionCallback callback);

    // Feed live sensor data in; only triggers a redraw if the value changed
    void setTemperature(float temperatureCelsius);
}