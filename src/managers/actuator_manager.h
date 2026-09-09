#pragma once

namespace actuator_manager
{
    void initialize();
    void open_valve_by_percent(float opening_percentage);
    void open_valve_by_degree(int degree);
    void close_valve();
    void open_wide_valve();
    void open_narrow_valve();
    bool is_valve_open();
    int get_current_valve_degree();
    bool is_valve_max_opening();

    
    void turn_on_pump();
    void turn_off_pump();
    bool is_pump_on();

    void turn_on_lighter();
    void turn_off_lighter();
    bool is_lighter_on();
}