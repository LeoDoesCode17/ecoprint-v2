#pragma once

namespace actuator_manager {
    void initialize();
    void open_valve_by_percent(unsigned int opening_percentage);
    void close_valve();
}