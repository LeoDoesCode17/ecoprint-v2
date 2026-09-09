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
    static int _current_valve_degree = 0;
    static bool _is_valve_max_opening = false;
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
        if (!_is_valve_open)
            _is_valve_open = true;
        unsigned int degree = (int)(constant::VALVE_PERCENT_TO_DEGREE_GRADIENT * opening_percentage + constant::VALVE_PERCENT_TO_DEGREE_OFFSET);
        servo_valve::open_by_degree(degree);
        // _is_valve_max_opening = ?
    }
    void close_valve()
    {
        if (_is_valve_open)
            _is_valve_open = false;
        servo_valve::close();
        _current_valve_degree = 0;
        _is_valve_max_opening = false;
    }
    void open_valve_by_degree(int degree)
    {
        if (!_is_valve_open)
            _is_valve_open = true;
        servo_valve::open_by_degree(degree);
        _current_valve_degree = degree;
        // _is_valve_max_opening = ?
    }
    void open_wide_valve()
    {
        if (!_is_valve_open)
            _is_valve_open = true;
        servo_valve::open_by_degree(30);
        _current_valve_degree = 30;
        _is_valve_max_opening = true;
    }
    void open_narrow_valve()
    {
        if (!_is_valve_open)
            _is_valve_open = true;
        servo_valve::open_by_degree(9);
        _current_valve_degree = 9;
        _is_valve_max_opening = false;
    }
    bool is_valve_open()
    {
        return _is_valve_open;
    }
    int get_current_valve_degree()
    {
        return _current_valve_degree;
    }
    bool is_valve_max_opening()
    {
        return _is_valve_max_opening;
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