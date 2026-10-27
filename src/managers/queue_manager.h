#pragma once
#include "config/type.h"

namespace queue_manager
{
    void initialize();
    bool enqueue_outbound(const outbound_msg_t &msg);
    bool dequeque_outbound(outbound_msg_t &msg, TickType_t timeout);
    bool enqueue_sensor_data(const sensor_t &data);
    bool enqueue_actuator_data(const actuator_t &data);
    bool enqueue_device_status_data(const device_status_t &data);
    bool enqueue_command_data(const command_t &cmd); // never blocks
    bool dequeue_command_data(command_t &cmd, TickType_t timeout);
    bool enqueue_command_event(const command_t &cmd);
    bool enqueue_command_rejected(const command_t &cmd, reject_reason_t why, StateMachine state);
}