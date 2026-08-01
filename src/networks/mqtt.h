#pragma once
#include <stdint.h>
#include "config/type.h"
namespace mqtt {
    void initialize();
    void connect_or_reconnect();
    void loop();
    bool is_connected();
    void set_callback(mqtt_callback callback);
    bool publish_message(const char* topic, const char* payload);
}