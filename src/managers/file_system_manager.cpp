#include "file_system_manager.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "network_manager.h"

namespace
{
    static const char *CONFIG_PATH = "/network-config.json";

    // Buffer sizes match the credential buffers in network_manager.cpp
    // (32/63-character limits + terminator)
    constexpr size_t kSsidSize = 33;
    constexpr size_t kWifiPasswordSize = 64;
    constexpr size_t kHostSize = 64;
    constexpr size_t kPortSize = 6;
    constexpr size_t kUsernameSize = 64;
    constexpr size_t kMqttPasswordSize = 64;

    // Storage for the strings that get_config() hands out. network_config_t
    // only holds pointers, and the JSON document is gone when get_config()
    // returns, so the text has to be copied somewhere that lives on.
    static char _ssid[kSsidSize];
    static char _wifi_password[kWifiPasswordSize];
    static char _host[kHostSize];
    static char _port[kPortSize];
    static char _username[kUsernameSize];
    static char _mqtt_password[kMqttPasswordSize];

    static bool _is_mounted = false;

    // Copies doc[key] into dest (truncating if needed). A missing key or an
    // unexpected type gives an empty string, which means "not set".
    static void copy_string(const JsonDocument &doc, const char *key, char *dest, size_t dest_size)
    {
        JsonVariantConst value = doc[key];

        if (value.is<const char *>())
        {
            snprintf(dest, dest_size, "%s", value.as<const char *>());
        }
        else if (value.is<long>())
        {
            // e.g. a hand-edited file with "mqtt_port": 1883 instead of "1883"
            snprintf(dest, dest_size, "%ld", value.as<long>());
        }
        else
        {
            dest[0] = '\0';
        }
    }
}
namespace file_system_manager
{

    void initalize()
    {
        // true = format the partition if it cannot be mounted. That is what
        // happens on the very first boot (blank flash), and it takes a moment.
        if (!LittleFS.begin(true))
        {
            Serial.println("[FileSystemManager] LittleFS mount failed - network config will not be loaded or saved");
            return;
        }
        _is_mounted = true;

        if (!LittleFS.exists(CONFIG_PATH))
        {
            Serial.printf("[FileSystemManager] %s not found, keeping default network config\n", CONFIG_PATH);
            return;
        }

        network_config_t config;
        if (!get_config(config))
        {
            Serial.println("[FileSystemManager] Saved network config is unreadable, keeping defaults");
            return;
        }

        // Seed the network manager's credential buffers
        network_manager::set_wifi_ssid(config.wifi_config.ssid);
        network_manager::set_wifi_password(config.wifi_config.password);
        network_manager::set_mqtt_broker_host(config.mqtt_config.host);
        network_manager::set_mqtt_port(config.mqtt_config.port);
        network_manager::set_mqtt_username(config.mqtt_config.username);
        network_manager::set_mqtt_password(config.mqtt_config.password);

        // Passwords are deliberately not logged
        Serial.printf("[FileSystemManager] Loaded network config (ssid: \"%s\", broker: %s:%s)\n",
                      config.wifi_config.ssid, config.mqtt_config.host, config.mqtt_config.port);
    }

    bool store_config(const network_config_t &network_config)
    {
        JsonDocument doc;
        doc["wifi_ssid"] = network_config.wifi_config.ssid;
        doc["wifi_password"] = network_config.wifi_config.password;
        doc["mqtt_broker_host"] = network_config.mqtt_config.host;
        doc["mqtt_port"] = network_config.mqtt_config.port;
        doc["mqtt_username"] = network_config.mqtt_config.username;
        doc["mqtt_password"] = network_config.mqtt_config.password;

        File f = LittleFS.open(CONFIG_PATH, FILE_WRITE); // truncate with old file
        if (!f)
        {
            Serial.printf("[FileSystemManager] Failed to open file %s\n", CONFIG_PATH);
            return false;
        }

        serializeJson(doc, f);
        f.close();
        Serial.printf("[FileSystemManager] Succeed to open and write file %s\n", CONFIG_PATH);
        return true;
    }

    bool get_config(network_config_t &out)
    {
        if (!_is_mounted || !LittleFS.exists(CONFIG_PATH))
        {
            return false;
        }

        File f = LittleFS.open(CONFIG_PATH, FILE_READ);
        if (!f)
        {
            Serial.printf("[FileSystemManager] Failed to open file %s\n", CONFIG_PATH);
            return false;
        }

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, f);
        f.close();

        if (err)
        {
            Serial.printf("[FileSystemManager] Failed to deserialize json with error %s\n", err.c_str());
            return false;
        }

        copy_string(doc, "wifi_ssid", _ssid, sizeof(_ssid));
        copy_string(doc, "wifi_password", _wifi_password, sizeof(_wifi_password));
        copy_string(doc, "mqtt_broker_host", _host, sizeof(_host));
        copy_string(doc, "mqtt_port", _port, sizeof(_port));
        copy_string(doc, "mqtt_username", _username, sizeof(_username));
        copy_string(doc, "mqtt_password", _mqtt_password, sizeof(_mqtt_password));

        out.wifi_config.ssid = _ssid;
        out.wifi_config.password = _wifi_password;
        out.mqtt_config.host = _host;
        out.mqtt_config.port = _port;
        out.mqtt_config.username = _username;
        out.mqtt_config.password = _mqtt_password;
        return true;
    }
}