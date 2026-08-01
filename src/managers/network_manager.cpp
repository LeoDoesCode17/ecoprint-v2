#include "network_manager.h"
#include "networks/wifi.h"
#include "networks/mqtt.h"
#include "config/constants.h"
#include <Arduino.h>
#include <ArduinoJson.h>

namespace
{
    static const int SENSOR_MESSAGE_BUFFER_SIZE = 128;
    static const int STATUS_MESSAGE_BUFFER_SIZE = 64;
}
namespace network_manager
{
    void initialize()
    {
        wifi::connect_or_reconnect();
        mqtt::initialize();
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

        StaticJsonDocument<SENSOR_MESSAGE_BUFFER_SIZE> doc;
        doc["water_temperature_celcius"] = water_temperature;
        doc["air_temperature_celcius"] = air_temperature;
        doc["air_humidity_percent"] = air_humidity;
        doc["is_water_sufficient"] = is_water_sufficient;

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
}