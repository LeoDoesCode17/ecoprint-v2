#pragma once
#include "config/type.h"
#include <stdint.h>
namespace network_manager {
    void initialize();
    void conect_or_reconnect();
    void publish_device_status(ecoprint_device_t device_status);
    void publish_sensor_data(ecoprint_sensor_t sensor_data);
    void publish_actuator_data(ecoprint_actuator_t &actuator_data);
    void mqtt_loop();
    void publish_ema_filtered_sensor_data(ema_filter_sensor_data_t sensor_data);
    void publish_stop_message(ecoprint_command_t command_data);
}