#pragma once
#include "config/type.h"

namespace file_system_manager
{
    // Mounts LittleFS and, if /network-config.json exists, copies its values
    // into the network manager's credential buffers (set_wifi_ssid(), ...).
    // Call once in setup(), BEFORE network_manager::initialize().
    void initalize();

    // Writes the config to /network-config.json (replacing the old file).
    bool store_config(const network_config_t &network_config);

    // Reads /network-config.json into `out`. Returns false, leaving `out`
    // untouched, if the file is missing or cannot be parsed.
    //
    // The strings `out` points to live in internal buffers: they stay valid
    // until the next get_config() call, so copy them if you need to keep them.
    bool get_config(network_config_t &out);
}