#pragma once
#include <Arduino.h>

namespace float_switch
{
    void initialize();
    int get_max_float_switch_state();
    int get_min_float_switch_state();
} // namespace float_switch
