#pragma once
#include "config/type.h"
#include <stdint.h>
namespace network_manager
{
    void initialize();
    void conect_or_reconnect();
    void publish_device_status(ecoprint_device_t device_status);
    void publish_sensor_data(ecoprint_sensor_t sensor_data);
    void publish_actuator_data(ecoprint_actuator_t &actuator_data);
    void mqtt_loop();
    void publish_ema_filtered_sensor_data(ema_filter_sensor_data_t sensor_data);
    void publish_stop_message(ecoprint_command_t command_data);
    long get_message_seq_id(int index);
    void reset_message_seq_id();
    void publish_start_message(ecoprint_start_command_t start_command_data);
    bool is_connected();
    bool publish(const outbound_msg_t &msg);
    void set_wifi_ssid(const char *ssid);
    void set_wifi_password(const char *password);
    void set_mqtt_broker_host(const char *host);
    void set_mqtt_port(const char *port);
    void set_mqtt_username(const char *username);
    void set_mqtt_password(const char *password);

    // Current runtime credentials (never nullptr; empty string = not set).
    // The pointers refer to internal buffers - copy them if you need to keep them.
    const char *get_wifi_ssid();
    const char *get_wifi_password();
    const char *get_mqtt_broker_host();
    const char *get_mqtt_port();
    const char *get_mqtt_username();
    const char *get_mqtt_password();
}