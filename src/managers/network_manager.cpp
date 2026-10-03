#include "network_manager.h"
#include "networks/wifi.h"
#include "networks/mqtt.h"
#include "config/constants.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include "actuator_manager.h"
#include "state_manager.h"

namespace
{
    static const int SENSOR_MESSAGE_BUFFER_SIZE = 256;
    static const int ACTUATOR_MESSAGE_BUFFER_SIZE = 256;
    static const int STATUS_MESSAGE_BUFFER_SIZE = 128;
    static const int COMMAND_MESSAGE_BUFFER_SIZE = 256;
    static const int ISO8601_BUFFER_SIZE = 26;
    constexpr size_t _TOPIC_BUF_SIZE = 64;
    static char mac[constant::MAC_BUF_SIZE];
    static char ECOPRINT_PUBLISH_SENSORS_TOPIC[_TOPIC_BUF_SIZE];
    static char ECOPRINT_PUBLISH_ACTUATORS_TOPIC[_TOPIC_BUF_SIZE];
    static char ECOPRINT_PUBLISH_STOP_COMMAND_TOPIC[_TOPIC_BUF_SIZE];
    static char ECOPRINT_EMA_FILTER_PUBLISH_SENSORS_TOPIC[_TOPIC_BUF_SIZE];
    static char ECOPRINT_SUBSCRIBE_START_COMMAND_TOPIC[_TOPIC_BUF_SIZE];

    // for packet loss data
    static unsigned long sensor_message_seq_id = 0;
    static unsigned long actuator_message_seq_id = 0;
    static unsigned long device_status_message_seq_id = 0;

    static void init_time()
    {
        // gmt_offset_sec = 0
        // daylight_offset_sec = 0
        // server 1 = "pool.ntp.org"
        // server 2 (if 1 fails) = "time.nist.gov"
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

        // Step 1: write "2026-08-02T10:23:01" (19 chars) into out
        size_t written = strftime(out, len, "%Y-%m-%dT%H:%M:%S", &timeinfo);
        if (written == 0)
            return false;

        // Step 2: append ".123Z" — tv_usec is microseconds, divide to get ms
        int ms = tv.tv_usec / 1000;
        snprintf(out + written, len - written, ".%03dZ", ms);

        return true;
    }
    static void read_esp32_mac_address()
    {
        wifi::copy_mac_address(mac, sizeof(mac));
        Serial.print("[WIFI] ESP32 MAC address is: ");
        Serial.println(mac);
    }

    static void build_sensors_publish_topic()
    {
        snprintf(ECOPRINT_PUBLISH_SENSORS_TOPIC, sizeof(ECOPRINT_PUBLISH_SENSORS_TOPIC), "esp/%s/telemetry/sensor", mac);
        snprintf(ECOPRINT_PUBLISH_ACTUATORS_TOPIC, sizeof(ECOPRINT_PUBLISH_ACTUATORS_TOPIC), "esp/%s/telemetry/actuator", mac);
        snprintf(ECOPRINT_EMA_FILTER_PUBLISH_SENSORS_TOPIC, sizeof(ECOPRINT_EMA_FILTER_PUBLISH_SENSORS_TOPIC), "esp/%s/ema/telemetry", mac);
        snprintf(ECOPRINT_PUBLISH_STOP_COMMAND_TOPIC, sizeof(ECOPRINT_PUBLISH_STOP_COMMAND_TOPIC), "esp/%s/command/stop", mac);
        Serial.print("[MQTT] Sensors publish topic: ");
        Serial.println(ECOPRINT_PUBLISH_SENSORS_TOPIC);
        Serial.print("[MQTT] Actuators publish topic: ");
        Serial.println(ECOPRINT_PUBLISH_ACTUATORS_TOPIC);
        Serial.print("[MQTT] EMA filtered publish topic: ");
        Serial.println(ECOPRINT_EMA_FILTER_PUBLISH_SENSORS_TOPIC);
        Serial.print("[MQTT] Stop command publish topic: ");
        Serial.println(ECOPRINT_PUBLISH_STOP_COMMAND_TOPIC);
    }

    static void build_subscribe_topic()
    {
        snprintf(ECOPRINT_SUBSCRIBE_START_COMMAND_TOPIC, sizeof(ECOPRINT_SUBSCRIBE_START_COMMAND_TOPIC), "esp/%s/command/start", mac);
        Serial.print("[MQTT] Start subscribe topic: ");
        Serial.println(ECOPRINT_SUBSCRIBE_START_COMMAND_TOPIC);
    }

    static void on_mqtt_message_callback(char *topic, byte *payload, unsigned int length)
    {
        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, payload, length);
        if (err)
        {
            Serial.printf("[MQTT] JSON parse failed: %s\n", err.c_str());
            return;
        }
        const int command = doc["command"];
        Serial.printf("[MQTT] Received message command from flutter command %d\n", command);
        if (command == 0)
        {
            state_manager::set_state_machine(StateMachine::IDLE);
        }
        else if (command == 1)
        {
            state_manager::set_state_machine(StateMachine::PREPARATION);
        }
        else
        {
            Serial.printf("[MQTT] Undefined receive command %d\n", command);
        }
    }

}
namespace network_manager
{
    void initialize()
    {
        wifi::connect_or_reconnect();
        mqtt::initialize();
        init_time();
        read_esp32_mac_address();
        build_sensors_publish_topic();
        build_subscribe_topic();
        mqtt::set_callback(on_mqtt_message_callback);
        mqtt::subscribe_to_topic(ECOPRINT_SUBSCRIBE_START_COMMAND_TOPIC);
    }
    void conect_or_reconnect()
    {
        wifi::connect_or_reconnect();
        mqtt::connect_or_reconnect();
    }
    void publish_device_status(ecoprint_device_t device_status)
    {
        const bool is_active = device_status.is_active;
        const int state_machine = static_cast<uint8_t>(device_status.state_machine);

        char recorded_at[ISO8601_BUFFER_SIZE];
        if (!get_iso8601_utc(recorded_at, sizeof(recorded_at)))
        {
            strcpy(recorded_at, "1970-01-01T00:00:00Z");
        }

        StaticJsonDocument<STATUS_MESSAGE_BUFFER_SIZE> doc;
        doc["is_active"] = is_active;
        doc["state_machine"] = state_machine;
        doc["recorded_at"] = recorded_at;

        // increment device_status seq id
        device_status_message_seq_id++;
        doc["seq_id"] = device_status_message_seq_id;

        char payload[STATUS_MESSAGE_BUFFER_SIZE];
        serializeJson(doc, payload);

        bool is_published = mqtt::publish_message(constant::PUBLISH_STATUS_TOPIC, payload);
        if (is_published)
        {
            Serial.printf("[MQTT]: SUCCESS TO PUBLISH %s TO TOPIC %s\n", payload, constant::PUBLISH_STATUS_TOPIC);
        }
        else
        {
            Serial.printf("[MQTT]: FAIL TO PUBLISH %s TO TOPIC %s\n", payload, constant::PUBLISH_STATUS_TOPIC);
        }
    }
    void publish_sensor_data(ecoprint_sensor_t sensor_data)
    {
        const float water_temperature = sensor_data.water_temperature;
        const float air_temperature = sensor_data.air_temperature;
        const float air_humidity = sensor_data.humidity;
        const bool is_water_sufficient = sensor_data.is_water_sufficient;
        const bool is_fire_on = sensor_data.is_fire_on;
        const float setpoint_temperature = sensor_data.setpoint;
        char recorded_at[ISO8601_BUFFER_SIZE];
        if (!get_iso8601_utc(recorded_at, sizeof(recorded_at)))
        {
            strcpy(recorded_at, "1970-01-01T00:00:00Z");
        }

        StaticJsonDocument<SENSOR_MESSAGE_BUFFER_SIZE> doc;
        doc["water_temperature"] = water_temperature;
        doc["air_temperature"] = air_temperature;
        doc["humidity"] = air_humidity;
        doc["is_water_sufficient"] = is_water_sufficient;
        doc["recorded_at"] = recorded_at;
        doc["setpoint"] = setpoint_temperature;
        doc["is_fire_on"] = is_fire_on;

        // increment sensor seq id
        sensor_message_seq_id++;
        doc["seq_id"] = sensor_message_seq_id;

        char payload[SENSOR_MESSAGE_BUFFER_SIZE];
        serializeJson(doc, payload);

        bool is_published = mqtt::publish_message(ECOPRINT_PUBLISH_SENSORS_TOPIC, payload);
        if (is_published)
        {
            Serial.printf("[MQTT]: SUCCESS TO PUBLISH %s TO TOPIC %s\n", payload, ECOPRINT_PUBLISH_SENSORS_TOPIC);
        }
        else
        {
            Serial.printf("[MQTT]: FAIL TO PUBLISH %s TO TOPIC %s\n", payload, ECOPRINT_PUBLISH_SENSORS_TOPIC);
        }
    }
    void publish_actuator_data(ecoprint_actuator_t &actuator_data)
    {
        char recorded_at[ISO8601_BUFFER_SIZE];
        if (!get_iso8601_utc(recorded_at, sizeof(recorded_at)))
        {
            strcpy(recorded_at, "1970-01-01T00:00:00Z");
        }
        JsonDocument doc;
        doc["valve_degree"] = actuator_data.valve_degree;
        doc["is_valve_open"] = actuator_data.is_valve_open;
        doc["is_max_valve_opening"] = actuator_data.is_max_valve_opening;
        doc["is_pump_on"] = actuator_data.is_pump_on;
        doc["is_lighter_on"] = actuator_data.is_lighter_on;
        doc["setpoint"] = actuator_data.setpoint;
        doc["recorded_at"] = recorded_at;

        // increment actuator seq id
        actuator_message_seq_id++;
        doc["seq_id"] = actuator_message_seq_id;

        char payload[ACTUATOR_MESSAGE_BUFFER_SIZE];
        serializeJson(doc, payload);

        bool is_published = mqtt::publish_message(ECOPRINT_PUBLISH_ACTUATORS_TOPIC, payload);
        if (is_published)
        {
            Serial.printf("[MQTT]: SUCCESS TO PUBLISH %s TO TOPIC %s\n", payload, ECOPRINT_PUBLISH_ACTUATORS_TOPIC);
        }
        else
        {
            Serial.printf("[MQTT]: FAIL TO PUBLISH %s TO TOPIC %s\n", payload, ECOPRINT_PUBLISH_ACTUATORS_TOPIC);
        }
    }
    void publish_ema_filtered_sensor_data(ema_filter_sensor_data_t sensor_data)
    {
        const float water_temperature = sensor_data.water_temperature;
        const float smoothing_factor = sensor_data.smoothing_factor;
        char recorded_at[ISO8601_BUFFER_SIZE];
        if (!get_iso8601_utc(recorded_at, sizeof(recorded_at)))
        {
            strcpy(recorded_at, "1970-01-01T00:00:00Z");
        }

        StaticJsonDocument<SENSOR_MESSAGE_BUFFER_SIZE> doc;
        doc["water_temperature"] = water_temperature;
        doc["smoothing_factor"] = smoothing_factor;
        doc["recorded_at"] = recorded_at;

        char payload[SENSOR_MESSAGE_BUFFER_SIZE];
        serializeJson(doc, payload);

        bool is_published = mqtt::publish_message(ECOPRINT_EMA_FILTER_PUBLISH_SENSORS_TOPIC, payload);
        if (is_published)
        {
            Serial.printf("[MQTT]: SUCCESS TO PUBLISH %s TO TOPIC %s\n", payload, ECOPRINT_EMA_FILTER_PUBLISH_SENSORS_TOPIC);
        }
        else
        {
            Serial.printf("[MQTT]: FAIL TO PUBLISH %s TO TOPIC %s\n", payload, ECOPRINT_EMA_FILTER_PUBLISH_SENSORS_TOPIC);
        }
    }

    void mqtt_loop()
    {
        mqtt::loop();
    }

    void publish_stop_message(ecoprint_command_t command_data)
    {
        char recorded_at[ISO8601_BUFFER_SIZE];
        if (!get_iso8601_utc(recorded_at, sizeof(recorded_at)))
        {
            strcpy(recorded_at, "1970-01-01T00:00:00Z");
        }
        StaticJsonDocument<SENSOR_MESSAGE_BUFFER_SIZE> doc;
        doc["recorded_at"] = recorded_at;
        doc["command"] = command_data.command;
        char payload[COMMAND_MESSAGE_BUFFER_SIZE];
        serializeJson(doc, payload);

        bool is_published = mqtt::publish_message(ECOPRINT_PUBLISH_STOP_COMMAND_TOPIC, payload);

        if (is_published)
        {
            Serial.printf("[MQTT]: SUCCESS TO PUBLISH %s TO TOPIC %s\n", payload, ECOPRINT_PUBLISH_STOP_COMMAND_TOPIC);
        }
        else
        {
            Serial.printf("[MQTT]: FAIL TO PUBLISH %s TO TOPIC %s\n", payload, ECOPRINT_PUBLISH_STOP_COMMAND_TOPIC);
        }
    }

    long get_message_seq_id(int index)
    {
        switch (index)
        {
        case 0:
            return sensor_message_seq_id;
            break;
        case 1:
            return actuator_message_seq_id;
            break;
        case 2:
            return device_status_message_seq_id;
            break;
        default:
            return constant::INVALID_SEQ_ID;
            break;
        }
    }

    void reset_message_seq_id()
    {
        sensor_message_seq_id = 0;
        actuator_message_seq_id = 0;
    }
}