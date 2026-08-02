#include "network_manager.h"
#include "networks/wifi.h"
#include "networks/mqtt.h"
#include "config/constants.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include "actuator_manager.h"

namespace
{
    static const int SENSOR_MESSAGE_BUFFER_SIZE = 200;
    static const int STATUS_MESSAGE_BUFFER_SIZE = 64;
    static const int ISO8601_BUFFER_SIZE = 26;
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

        // Step 1: write "2026-08-02T10:23:01" (19 chars) into out
        size_t written = strftime(out, len, "%Y-%m-%dT%H:%M:%S", &timeinfo);
        if (written == 0)
            return false;

        // Step 2: append ".123Z" — tv_usec is microseconds, divide to get ms
        int ms = tv.tv_usec / 1000;
        snprintf(out + written, len - written, ".%03dZ", ms);

        return true;
    }
    static void on_mqtt_message(char *topic, byte *payload, unsigned int length)
    {
        StaticJsonDocument<128> doc;
        DeserializationError err = deserializeJson(doc, payload, length);
        if (err)
        {
            Serial.printf("[MQTT] JSON parse failed: %s\n", err.c_str());
            return;
        }
        if (strcmp(topic, constant::SUBSCRIBE_ACTUATOR_TOPIC) == 0)
        {
            const char *actuator = doc["actuator"];
            const int value = doc["value"];
            if (!actuator)
            {
                Serial.println("[MQTT] Missing required fields");
                return;
            }
            if (strcmp(actuator, "servo_valve") == 0) {
                actuator_manager::open_valve_by_percent(value);
                Serial.printf("[ACTUATOR] Opening servo valve by %d%\n", value);
            }
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
        mqtt::set_callback(on_mqtt_message);
    }
    void conect_or_reconnect()
    {
        wifi::connect_or_reconnect();
        mqtt::connect_or_reconnect();
    }
    void publish_device_status(ecoprint_device_t device_status)
    {
        const bool is_active = device_status.is_active;
        const int device_state_machine = device_status.state_machine;

        StaticJsonDocument<STATUS_MESSAGE_BUFFER_SIZE> doc;
        doc["is_active"] = is_active;
        doc["device_state_machine"] = device_state_machine;

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
        const float air_humidity = sensor_data.air_humidity;
        const bool is_water_sufficient = sensor_data.is_water_sufficient;
        char recorded_at[ISO8601_BUFFER_SIZE];
        if (!get_iso8601_utc(recorded_at, sizeof(recorded_at)))
        {
            strcpy(recorded_at, "1970-01-01T00:00:00Z");
        }

        StaticJsonDocument<SENSOR_MESSAGE_BUFFER_SIZE> doc;
        doc["water_temperature_celcius"] = water_temperature;
        doc["air_temperature_celcius"] = air_temperature;
        doc["air_humidity_percent"] = air_humidity;
        doc["is_water_sufficient"] = is_water_sufficient;
        doc["recorded_at"] = recorded_at;

        char payload[SENSOR_MESSAGE_BUFFER_SIZE];
        serializeJson(doc, payload);

        bool is_published = mqtt::publish_message(constant::PUBLISH_SENSORS_TOPIC, payload);
        if (is_published)
        {
            Serial.printf("[MQTT]: SUCCESS TO PUBLISH %s TO TOPIC %s\n", payload, constant::PUBLISH_SENSORS_TOPIC);
        }
        else
        {
            Serial.printf("[MQTT]: FAIL TO PUBLISH %s TO TOPIC %s\n", payload, constant::PUBLISH_SENSORS_TOPIC);
        }
    }
    void mqtt_loop() {
        mqtt::loop();
    }
}