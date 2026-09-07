#include "actuator_manager.h"
#include "actuators/servo_valve.h"
#include "actuators/pump.h"
#include "actuators/lighter.h"
#include "config/constants.h"

namespace
{
    static bool _is_valve_open = false;
    static bool _is_pump_on = false;
    static bool _is_lighter_on = false;
} // namespace

namespace actuator_manager
{
    void initialize()
    {
        servo_valve::initialize();
        pump::initialize();
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
    bool is_valve_open()
    {
        return _is_valve_open;
    }
    void turn_on_pump()
    {
        pump::on();
        _is_pump_on = true;
    }
    void turn_off_pump()
    {
        pump::off();
        _is_pump_on = false;
    }
    bool is_pump_on()
    {
        return _is_pump_on;
    }

    void turn_on_lighter()
    {
        lighter::on();
        _is_lighter_on = true;
    }
    void turn_off_lighter()
    {
        lighter::off();
        _is_lighter_on = false;
    }
    bool is_lighter_on()
    {
        return _is_lighter_on;
    }
}