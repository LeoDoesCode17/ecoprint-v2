#pragma once
#include "config/type.h"
namespace network_manager {
    void initialize();
    void conect_or_reconnect();
    void publish_device_status(ecoprint_device_t device_status);
    void publish_sensor_data(ecoprint_sensor_t sensor_data);
    void mqtt_loop();
}