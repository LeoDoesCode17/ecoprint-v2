#pragma once
#include "config/type.h"
#include <stdint.h>
namespace network_manager
{
    void initialize();
    void conect_or_reconnect();
    void mqtt_loop();
    long get_message_seq_id(int index);
    void reset_message_seq_id();
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

    bool is_wifi_connected();
    bool is_mqtt_connected();
}