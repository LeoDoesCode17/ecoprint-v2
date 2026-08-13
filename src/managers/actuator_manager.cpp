#include "actuator_manager.h"
#include "actuators/servo_valve.h"
#include "config/constants.h"

namespace 
{
    static bool _is_valve_open = false;
} // namespace 

namespace actuator_manager
{
    void initialize()
    {
        servo_valve::initialize();
    }
    void open_valve_by_percent(float opening_percentage)
    {
        unsigned int degree = (int)(constant::VALVE_PERCENT_TO_DEGREE_GRADIENT * opening_percentage + constant::VALVE_PERCENT_TO_DEGREE_OFFSET);
        servo_valve::open_by_degree(degree);
    }
    void close_valve()
    {
        servo_valve::close();
        _is_valve_open = false;
    }
    void open_valve_by_degree(int degree)
    {
        servo_valve::open_by_degree(degree);
        _is_valve_open = true;
    }
    bool is_valve_open() {
        return _is_valve_open;
    }
}