#pragma once

namespace actuator_manager {
    void initialize();
    void open_valve_by_percent(float opening_percentage);
    void open_valve_by_degree(int degree);
    void close_valve();
    bool is_valve_open();
}