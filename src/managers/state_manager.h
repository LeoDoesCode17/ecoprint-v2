#pragma once
#include "config/type.h"

namespace state_manager {
    void set_state_machine(StateMachine state);
    StateMachine get_state_machine();
    void handle_command(const command_t &cmd);
}