#include "actuator_manager.h"
#include "actuators/servo_valve.h"
#include "config/constants.h"

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
    }
}