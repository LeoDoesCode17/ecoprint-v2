#pragma once

namespace actuator_manager {
    void initialize();
    void open_valve_by_percent(float opening_percentage);
    void close_valve();
}