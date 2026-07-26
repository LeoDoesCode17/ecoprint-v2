#pragma once

namespace servo_valve {
    void initialize();
    void open_by_degree(unsigned long degree);
    void close();
}