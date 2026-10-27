#include "state_manager.h"
#include "config/constants.h"
#include "queue_manager.h"
#include "sensor_manager.h"

namespace
{
    static StateMachine _state_machine = StateMachine::IDLE;

    static bool is_running(StateMachine state)
    {
        switch (state)
        {
        case StateMachine::PREPARATION:
        case StateMachine::FILLING_WATER:
        case StateMachine::FIRING:
        case StateMachine::HEATING:
        case StateMachine::STEAMING:
            return true;
        default:
            return false;
        }
    }

    static bool check_start(const command_t &cmd, StateMachine state, reject_reason_t &why)
    {
        if (cmd.setpoint < constant::MIN_SETPOINT_CELCIUS || cmd.setpoint > constant::MAX_SETPOINT_CELCIUS || cmd.timer_minutes < constant::MIN_TIMER_MINUTE || cmd.timer_minutes > constant::MAX_TIMER_MINUTE)
        {
            why = reject_reason_t::INVALID_VALUE;
            return false;
        }

        if (is_running(state))
        {
            why = reject_reason_t::ALREADY_RUNNING;
            return false;
        }

        if (state == StateMachine::FAILED || state == StateMachine::ERROR)
        {
            why = reject_reason_t::DEVICE_ERROR;
            return false;
        }

        return true;
    }

} // namespace

namespace state_manager
{
    void set_state_machine(StateMachine state)
    {
        _state_machine = state;
    }

    StateMachine get_state_machine()
    {
        return _state_machine;
    }

    void handle_command(const command_t &cmd)
    {
        const StateMachine current_state_machine = get_state_machine();

        if (cmd.command == 0)
        {
            // always allowed (or add checking here)
            set_state_machine(StateMachine::COMPLETED);
            queue_manager::enqueue_command_event(cmd);
            return;
        }

        reject_reason_t why;
        if (!check_start(cmd, current_state_machine, why))
        {
            Serial.printf("[CTRL] Start rejected (reason %d, state %d)\n", static_cast<int>(why), static_cast<int>(current_state_machine));
            if (cmd.source == command_source_t::REMOTE) // the app needs to know
                queue_manager::enqueue_command_rejected(cmd, why, current_state_machine);
            return;
        }

        sensor_manager::set_setpoint_temperature((float)cmd.setpoint);
        sensor_manager::set_timer(cmd.timer_minutes);
        state_manager::set_state_machine(StateMachine::PREPARATION); // state changes NOW
        queue_manager::enqueue_command_event(cmd);
    }

} // namespace state_manager
