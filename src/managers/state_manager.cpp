#include "state_manager.h"

namespace
{
    static StateMachine _state_machine = StateMachine::IDLE;
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

} // namespace state_manager
