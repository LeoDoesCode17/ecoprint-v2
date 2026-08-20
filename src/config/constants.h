#pragma once
namespace constant
{
    const unsigned int ZERO_PERCENT_DEGREE = 0;
    // const unsigned int ZERO_PERCENT_DEGREE = 9;
    const unsigned int VALVE_PERCENT_TO_DEGREE_GRADIENT = 94;
    const unsigned int VALVE_PERCENT_TO_DEGREE_OFFSET = 6;
    const char WIFI_SSID[] = "WIFI_SSID";
    const char WIFI_PASSWORD[] = "WIFI_PASSWORD";
    const char MQTT_BROKER_MDNS[] = "raspberrypi";
    const char ECOPRINT_MQTT_HOST[] = "ecoprint.vpspenelitian.com";
    const int MQTT_PORT = 1883;
    const int RASPI_MQTT_PORT = 1884;
    const unsigned long MSG_BUFFER_SIZE = 100;
    const char MQTT_CLIENT_ID[] = "ESP32";
    const char ECOPRINT_MQTT_CLIENT_ID[] = "esp_client";
    const char SUBSCRIBE_TOPIC[] = "mqtt/first";
    const char PUBLISH_TOPIC[] = "mqtt/second";
    const char PUBLISH_STATUS_TOPIC[] = "ecoprint/mac-address/status";  
    const char PUBLISH_SENSORS_TOPIC[] = "ecoprint/mac-address/sensors";
    const char SUBCRIBE_COMMAND_TOPIC[] = "ecoprint/mac-address/command";
    const char SUBSCRIBE_ACTUATOR_TOPIC[] = "ecoprint/mac-address/actuator";
    const char SUBSCRIBE_CONFIG_TOPIC[] = "ecoprint/mac-address/config";

    const char ECOPRINT_PUBLISH_SENSORS_TOPIC[] = "esp/F4:65:0B:54:B0:54/telemetry";
    const char ECOPRINT_SUBSCRIBE_COMMAND_TOPIC[] = "esp/F4:65:0B:54:B0:54/command";

    const int MAC_BUF_SIZE = 18;

    const unsigned long MAIN_LOOP_INTERVAL_MS = 10;
    const unsigned long PUBLISH_SENSOR_DATA_INTERVAL_MS = 9990;
    const unsigned long UPDATE_SENSOR_DATA_INTERVAL_MS = 1990;
    const unsigned long PUBLISH_DEVICE_STATUS_INTERVAL_MS = 10000;

    const int EMA_SMOOTHING_FACTOR_SIZE = 5;
    const float EMA_SMOOTHING_FACTOR[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};

    const float UPPER_HYSTERESIS_BAND = 1.0f;
    const float LOWER_HYSTERESIS_BAND = 1.0f;
    const float THERMOCOUPLE_OFFSET = 6.0f;

    const int SHT3X_ADDR = 0x44;
}