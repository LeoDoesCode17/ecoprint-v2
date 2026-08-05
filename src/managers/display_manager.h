#pragma once
#include <Arduino.h>
#include "config/type.h"

namespace display_manager
{
    // Initializes the TFT, draws the title, and draws the initial button layout.
    void initialize();

    // Call every loop() iteration. Pass the raw encoder position and the
    // debounced button-press edge from sensor_manager. Handles selection
    // highlighting and dispatches the action for the selected button.
    void update(long encoderPosition, bool buttonPressed);

    void showSensorScreen(const ecoprint_sensor_t &data);
}
