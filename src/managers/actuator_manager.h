#pragma once

namespace actuator_manager
{
    void initialize();
    void open_valve_by_percent(float opening_percentage);
    void open_valve_by_degree(int degree);
    void close_valve();
    bool is_valve_open();
    
    void turn_on_pump();
    void turn_off_pump();
    bool is_pump_on();

    void turn_on_lighter();
    void turn_off_lighter();
    bool is_lighter_on();
}