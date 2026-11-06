#include "network_manager.h"
#include "networks/wifi.h"
#include "networks/mqtt.h"
#include "config/constants.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include "actuator_manager.h"
#include "state_manager.h"
#include "sensor_manager.h"
#include "queue_manager.h"

namespace
{
    static const int COMMAND_MESSAGE_BUFFER_SIZE = 256;
    static const int OUTBOUND_MESSAGE_BUFFER_SIZE = 256;
    static const int ISO8601_BUFFER_SIZE = 26;
    constexpr size_t _TOPIC_BUF_SIZE = 64;
    static char mac[constant::MAC_BUF_SIZE];

    static char ECOPRINT_PUBLISH_SENSORS_TOPIC[_TOPIC_BUF_SIZE];
    static char ECOPRINT_PUBLISH_ACTUATORS_TOPIC[_TOPIC_BUF_SIZE];
    static char ECOPRINT_PUBLISH_EVENT_TOPIC[_TOPIC_BUF_SIZE];
    static char ECOPRINT_PUBLISH_DEVICE_STATUS_TOPIC[_TOPIC_BUF_SIZE];
    static char ECOPRINT_SUBSCRIBE_COMMAND_TOPIC[_TOPIC_BUF_SIZE];

    // Sequence IDs for packet loss tracking
    static unsigned long sensor_message_seq_id = 0;
    static unsigned long actuator_message_seq_id = 0;
    static unsigned long device_status_message_seq_id = 0;

    // wifi credential buffer
    static const int WIFI_SSID_BUFFER_SIZE = 33; // maximum length is 32 chars
    static const int WIFI_PASS_BUFFER_SIZE = 64; // maximum length is 63 chars
    static char _WIFI_SSID[WIFI_SSID_BUFFER_SIZE];
    static char _WIFI_PASSWORD[WIFI_PASS_BUFFER_SIZE];

    // mqtt credential buffer
    static const int MQTT_BROKER_HOST_BUFFER_SIZE = 64;
    static const int MQTT_BROKER_PORT_BUFFER_SIZE = 6; // from 0 - 65535 ports
    static const int MQTT_USERNAME_BUFFER_SIZE = 64;
    static const int MQTT_PASSWORD_BUFFER_SIZE = 64;
    static char _MQTT_BROKER_HOST[MQTT_BROKER_HOST_BUFFER_SIZE];
    static char _MQTT_BROKER_PORT[MQTT_BROKER_PORT_BUFFER_SIZE];
    static char _MQTT_USERNAME[MQTT_USERNAME_BUFFER_SIZE];
    static char _MQTT_PASSWORD[MQTT_PASSWORD_BUFFER_SIZE];

    static void init_time()
    {
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");

        Serial.print("[TIME] Syncing");
        time_t now = time(nullptr);
        int retry = 0;
        while (now <= 100000 && retry < 20)
        {
            delay(500);
            Serial.print(".");
            now = time(nullptr);
            retry++;
        }
        Serial.println();

        if (now > 100000)
        {
            Serial.println("[TIME] Time synchronized");
        }
        else
        {
            Serial.println("[TIME] Time sync failed");
        }
    }

    static bool get_iso8601_utc(char *out, size_t len)
    {
        if (len < ISO8601_BUFFER_SIZE)
            return false;

        struct timeval tv;
        gettimeofday(&tv, nullptr); // seconds + microseconds

        if (tv.tv_sec <= 100000)
            return false;

        struct tm timeinfo;
        gmtime_r(&tv.tv_sec, &timeinfo);

        size_t written = strftime(out, len, "%Y-%m-%dT%H:%M:%S", &timeinfo);
        if (written == 0)
            return false;

        int ms = tv.tv_usec / 1000;
        snprintf(out + written, len - written, ".%03dZ", ms);

        return true;
    }

    static void read_esp32_mac_address()
    {
        uint64_t mac_result = ESP.getEfuseMac();
        uint8_t _mac[6];
        _mac[0] = (mac_result >> 0) & 0xFF;
        _mac[1] = (mac_result >> 8) & 0xFF;
        _mac[2] = (mac_result >> 16) & 0xFF;
        _mac[3] = (mac_result >> 24) & 0xFF;
        _mac[4] = (mac_result >> 32) & 0xFF;
        _mac[5] = (mac_result >> 40) & 0xFF;
        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                 _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
        Serial.print("[WIFI] ESP32 MAC address is: ");
        Serial.println(mac);
    }

    static void build_publish_topics()
    {
        snprintf(ECOPRINT_PUBLISH_SENSORS_TOPIC, sizeof(ECOPRINT_PUBLISH_SENSORS_TOPIC), "esp/%s/telemetry/sensor", mac);
        snprintf(ECOPRINT_PUBLISH_ACTUATORS_TOPIC, sizeof(ECOPRINT_PUBLISH_ACTUATORS_TOPIC), "esp/%s/telemetry/actuator", mac);
        snprintf(ECOPRINT_PUBLISH_DEVICE_STATUS_TOPIC, sizeof(ECOPRINT_PUBLISH_DEVICE_STATUS_TOPIC), "esp/%s/telemetry/status", mac);
        snprintf(ECOPRINT_PUBLISH_EVENT_TOPIC, sizeof(ECOPRINT_PUBLISH_EVENT_TOPIC), "esp/%s/event", mac);
        Serial.print("[MQTT] Sensors publish topic: ");
        Serial.println(ECOPRINT_PUBLISH_SENSORS_TOPIC);
        Serial.print("[MQTT] Actuators publish topic: ");
        Serial.println(ECOPRINT_PUBLISH_ACTUATORS_TOPIC);
        Serial.print("[MQTT] Telemetry status device publish topic: ");
        Serial.println(ECOPRINT_PUBLISH_DEVICE_STATUS_TOPIC);
        Serial.print("[MQTT] Event publish topic: ");
        Serial.println(ECOPRINT_PUBLISH_EVENT_TOPIC);
    }

    static void build_subscribe_topics()
    {
        snprintf(ECOPRINT_SUBSCRIBE_COMMAND_TOPIC, sizeof(ECOPRINT_SUBSCRIBE_COMMAND_TOPIC), "esp/%s/command", mac);
        Serial.print("[MQTT] Start subscribe command topic: ");
        Serial.println(ECOPRINT_SUBSCRIBE_COMMAND_TOPIC);
    }

    static const char *reason_str(reject_reason_t r)
    {
        switch (r)
        {
        case reject_reason_t::ALREADY_RUNNING:
            return "already_running";
        case reject_reason_t::DEVICE_ERROR:
            return "device_error";
        case reject_reason_t::INVALID_VALUE:
            return "invalid_value";
        }
        return "unknown";
    }

    static void on_mqtt_message_callback(char *topic, byte *payload, unsigned int length)
    {
        Serial.printf("[MQTT] Message arrived on [%s]\n", topic);

        StaticJsonDocument<COMMAND_MESSAGE_BUFFER_SIZE> doc;
        DeserializationError err = deserializeJson(doc, payload, length);
        if (err)
        {
            Serial.printf("[MQTT] JSON parse failed: %s\n", err.c_str());
            return;
        }

        if (strcmp(topic, ECOPRINT_SUBSCRIBE_COMMAND_TOPIC) == 0)
        {
            command_t command_data;
            Serial.println("[MQTT] Received command message");
            command_data.setpoint = doc["setpoint"] | -1;
            command_data.timer_minutes = doc["timer_minutes"] | -1;
            command_data.command = doc["command"] | -1;
            command_data.source = command_source_t::REMOTE;

            switch (command_data.command)
            {
            case 0:
                Serial.println("[MQTT] Receive command to stop steaming");
                if (!queue_manager::enqueue_command_data(command_data))
                {
                    Serial.println("[MQTT] Command queue full, command dropped");
                }
                break;
            case 1:
                Serial.println("[MQTT] Receive command to start steaming");
                if (!queue_manager::enqueue_command_data(command_data))
                {
                    Serial.println("[MQTT] Command queue full, command dropped");
                }
                break;
            default:
                Serial.println("[MQTT] Receive unknown command");
                break;
            }
        }
        else
        {
            return;
        }
    }
}

namespace network_manager
{
    void initialize()
    {
        sensor_message_seq_id = 0;
        actuator_message_seq_id = 0;
        device_status_message_seq_id = 0;

        read_esp32_mac_address(); // non blocking
        build_publish_topics();   // non blocking
        build_subscribe_topics(); // non blocking

        wifi::connect_or_reconnect(); // Blocking
        mqtt::initialize();
        mqtt::set_callback(on_mqtt_message_callback);

        // Ensure active MQTT connection BEFORE subscribing to topics
        mqtt::connect_or_reconnect();
        mqtt::subscribe_to_topic(ECOPRINT_SUBSCRIBE_COMMAND_TOPIC);

        init_time();
    }

    void conect_or_reconnect()
    {
        wifi::connect_or_reconnect();

        if (!mqtt::is_connected())
        {
            mqtt::connect_or_reconnect();
            // Resubscribe to dynamic macro topic upon reconnection
            mqtt::subscribe_to_topic(ECOPRINT_SUBSCRIBE_COMMAND_TOPIC);
        }
    }

    void mqtt_loop()
    {
        mqtt::loop();
    }

    long get_message_seq_id(int index)
    {
        switch (index)
        {
        case 0:
            return sensor_message_seq_id;
        case 1:
            return actuator_message_seq_id;
        case 2:
            return device_status_message_seq_id;
        default:
            return constant::INVALID_SEQ_ID;
        }
    }

    void reset_message_seq_id()
    {
        sensor_message_seq_id = 0;
        actuator_message_seq_id = 0;
        device_status_message_seq_id = 0;
    }

    bool is_connected()
    {
        return wifi::is_connected() && mqtt::is_connected();
    }

    bool publish(const outbound_msg_t &msg)
    {
        if (!mqtt::is_connected())
        {
            Serial.printf("[MQTT] Failed to publish outbound message type %d\n", static_cast<int>(msg.type));
            return false;
        }

        char recorded_at[ISO8601_BUFFER_SIZE];
        if (!get_iso8601_utc(recorded_at, sizeof(recorded_at)))
        {
            strcpy(recorded_at, "1970-01-01T00:00:00Z");
        }

        StaticJsonDocument<OUTBOUND_MESSAGE_BUFFER_SIZE> doc;

        switch (msg.type)
        {
        case msg_type_t::SENSOR:
        {
            doc["water_temperature"] = msg.sensor.water_temperature;
            doc["air_temperature"] = msg.sensor.air_temperature;
            doc["humidity"] = msg.sensor.humidity;
            doc["is_water_sufficient"] = msg.sensor.is_water_sufficient;
            doc["recorded_at"] = recorded_at;
            // doc["setpoint"] = msg.sensor.setpoint_temperature;
            doc["is_fire_on"] = msg.sensor.is_fire_on;

            sensor_message_seq_id++;
            doc["seq_id"] = sensor_message_seq_id;

            char payload[OUTBOUND_MESSAGE_BUFFER_SIZE];
            serializeJson(doc, payload);

            bool is_published = mqtt::publish_message(ECOPRINT_PUBLISH_SENSORS_TOPIC, payload);
            return is_published;
        }
        case msg_type_t::ACTUATOR:
        {
            doc["valve_degree"] = msg.actuator.valve_degree;
            doc["is_valve_open"] = msg.actuator.is_valve_open;
            doc["is_max_valve_opening"] = msg.actuator.is_max_valve_opening;
            doc["is_pump_on"] = msg.actuator.is_pump_on;
            doc["is_lighter_on"] = msg.actuator.is_lighter_on;
            // doc["setpoint"] = msg.actuator.setpoint;
            doc["recorded_at"] = recorded_at;

            actuator_message_seq_id++;
            doc["seq_id"] = actuator_message_seq_id;

            char payload[OUTBOUND_MESSAGE_BUFFER_SIZE];
            serializeJson(doc, payload);

            bool is_published = mqtt::publish_message(ECOPRINT_PUBLISH_ACTUATORS_TOPIC, payload);
            return is_published;
        }
        case msg_type_t::DEVICE_STATUS:
        {
            // doc["is_active"] = is_active;
            doc["state_machine"] = static_cast<int>(msg.device_status.state_machine);
            doc["recorded_at"] = recorded_at;

            device_status_message_seq_id++;
            doc["seq_id"] = device_status_message_seq_id;

            char payload[OUTBOUND_MESSAGE_BUFFER_SIZE];
            serializeJson(doc, payload);

            bool is_published = mqtt::publish_message(ECOPRINT_PUBLISH_DEVICE_STATUS_TOPIC, payload);
            return is_published;
        }
        case msg_type_t::COMMAND_EVENT:
        {
            doc["recorded_at"] = recorded_at;
            doc["event"] = msg.command_event.command == 1 ? "run_started" : "run_finished";
            doc["source"] = msg.command_event.source == command_source_t::LOCAL ? "local" : "remote";
            doc["setpoint"] = msg.command_event.setpoint;
            doc["timer_minutes"] = msg.command_event.timer_minutes;
            doc["is_allowed"] = 1; // 1 means the request event to control is accepted

            char payload[OUTBOUND_MESSAGE_BUFFER_SIZE];
            serializeJson(doc, payload);

            bool is_published = mqtt::publish_message(ECOPRINT_PUBLISH_EVENT_TOPIC, payload);
            return is_published;
        }
        case msg_type_t::COMMAND_REJECTED:
        {
            doc["recorded_at"] = recorded_at;
            doc["event"] = "command_rejected";
            doc["setpoint"] = msg.command_rejected.command.setpoint;
            doc["timer_minutes"] = msg.command_rejected.command.timer_minutes;
            doc["reason"] = reason_str(msg.command_rejected.reason);
            doc["state_machine"] = static_cast<int>(msg.command_rejected.state);
            doc["source"] = "remote";
            doc["command"] = msg.command_rejected.command.command;
            doc["is_allowed"] = 0; // 0 means the request event to control is rejected

            char payload[OUTBOUND_MESSAGE_BUFFER_SIZE];
            serializeJson(doc, payload);

            bool is_published = mqtt::publish_message(ECOPRINT_PUBLISH_EVENT_TOPIC, payload);
            return is_published;
        }
        default:
            break;
        }

        return false;
    }

    void set_wifi_ssid(const char *ssid)
    {
        snprintf(_WIFI_SSID, sizeof(_WIFI_SSID), "%s", ssid ? ssid : "");
    }

    void set_wifi_password(const char *password)
    {
        snprintf(_WIFI_PASSWORD, sizeof(_WIFI_PASSWORD), "%s", password ? password : "");
    }

    void set_mqtt_broker_host(const char *host)
    {
        snprintf(_MQTT_BROKER_HOST, sizeof(_MQTT_BROKER_HOST), "%s", host ? host : "");
    }

    void set_mqtt_port(const char *port)
    {
        snprintf(_MQTT_BROKER_PORT, sizeof(_MQTT_BROKER_PORT), "%s", port ? port : "");
    }

    void set_mqtt_username(const char *username)
    {
        snprintf(_MQTT_USERNAME, sizeof(_MQTT_USERNAME), "%s", username ? username : "");
    }

    void set_mqtt_password(const char *password)
    {
        snprintf(_MQTT_PASSWORD, sizeof(_MQTT_PASSWORD), "%s", password ? password : "");
    }

    const char *get_wifi_ssid()
    {
        return _WIFI_SSID;
    }

    const char *get_wifi_password()
    {
        return _WIFI_PASSWORD;
    }

    const char *get_mqtt_broker_host()
    {
        return _MQTT_BROKER_HOST;
    }

    const char *get_mqtt_port()
    {
        return _MQTT_BROKER_PORT;
    }

    const char *get_mqtt_username()
    {
        return _MQTT_USERNAME;
    }

    const char *get_mqtt_password()
    {
        return _MQTT_PASSWORD;
    }

}