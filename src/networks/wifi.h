#pragma once
namespace wifi {
    void initialize(const char* ssid, const char* password);
    void connect_or_reconnect();
    bool is_connected();
}