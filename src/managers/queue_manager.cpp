#include "queue_manager.h"

namespace
{
    constexpr UBaseType_t OUTBOUND_QUEUE_LENGTH = 10;
    constexpr UBaseType_t COMMAND_QUEUE_LENGTH = 5;
    QueueHandle_t outbound_queue = nullptr, command_queue = nullptr;
}

namespace queue_manager
{
    void initialize()
    {
        outbound_queue = xQueueCreate(OUTBOUND_QUEUE_LENGTH, sizeof(outbound_msg_t));
        command_queue = xQueueCreate(COMMAND_QUEUE_LENGTH, sizeof(command_t));

        if (!outbound_queue || !command_queue)
        {
            Serial.println("[FREE-RTOS] Creation queue failed! Halt system");
            while (true)
                delay(1000);
        }
    }

    bool enqueue_outbound(const outbound_msg_t &msg)
    {
        if (xQueueSend(outbound_queue, &msg, 0) == pdTRUE)
            return true;

        // Full: drop the oldest item, then retry once.
        outbound_msg_t discarded;
        xQueueReceive(outbound_queue, &discarded, 0);
        if (xQueueSend(outbound_queue, &msg, 0) == pdTRUE)
        {
            Serial.println("[FREE-RTOS] Outbound full, dropped oldest outbound message");
            return true;
        }
        return false;
    }

    bool dequeque_outbound(outbound_msg_t &msg, TickType_t timeout)
    {
        return xQueueReceive(outbound_queue, &msg, timeout) == pdTRUE;
    }

    bool enqueue_sensor_data(const sensor_t &data)
    {
        outbound_msg_t msg;
        msg.type = msg_type_t::SENSOR;
        msg.sensor = data;
        return enqueue_outbound(msg);
    }

    bool enqueue_actuator_data(const actuator_t &data)
    {
        outbound_msg_t msg;
        msg.type = msg_type_t::ACTUATOR;
        msg.actuator = data;
        return enqueue_outbound(msg);
    }

    bool enqueue_device_status_data(const device_status_t &data)
    {
        outbound_msg_t msg;
        msg.type = msg_type_t::DEVICE_STATUS;
        msg.device_status = data;
        return enqueue_outbound(msg);
    }

    // not used in any places
    bool enqueue_command_data(const command_t &cmd)
    {
        bool is_queue_send = xQueueSend(command_queue, &cmd, 0) == pdTRUE;
        return is_queue_send;
    }

    bool dequeue_command_data(command_t &cmd, TickType_t timeout)
    {
        return xQueueReceive(command_queue, &cmd, timeout) == pdTRUE;
    }

    bool enqueue_command_event(const command_t &cmd)
    {
        outbound_msg_t msg;
        msg.type = msg_type_t::COMMAND_EVENT;
        msg.command_event = cmd;
        return enqueue_outbound(msg);
    }

    bool enqueue_command_rejected(const command_t &cmd, reject_reason_t why, StateMachine state)
    {
        outbound_msg_t msg;
        msg.type = msg_type_t::COMMAND_REJECTED;
        msg.command_rejected = {cmd, why, state};
        return enqueue_outbound(msg);
    }

}