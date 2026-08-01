#pragma once
namespace constant
{
    const unsigned int ZERO_PERCENT_DEGREE = 6;
    const unsigned int VALVE_PERCENT_TO_DEGREE_GRADIENT = 94;
    const unsigned int VALVE_PERCENT_TO_DEGREE_OFFSET = 6;
    const char WIFI_SSID[] = "WIFI_SSID";
    const char WIFI_PASSWORD[] = "WIFI_PASSWORD";
    const char MQTT_BROKER_MDNS[] = "raspberrypi";
    const int MQTT_PORT = 1884;
    const unsigned long MSG_BUFFER_SIZE = 100;
    const char MQTT_CLIENT_ID[] = "ESP32";
    const char SUBSCRIBE_TOPIC[] = "mqtt/first";
    const char PUBLISH_TOPIC[] = "mqtt/second";
    const char PUBLISH_STATUS_TOPIC[] = "ecoprint/mac-address/status";  
    const char PUBLISH_SENSORS_TOPIC[] = "ecoprint/mac-address/sensors";
    const char SUBCRIBE_COMMAND_TOPIC[] = "ecoprint/mac-address/command";
    const char SUBSCRIBE_ACTUATOR_TOPIC[] = "ecoprint/mac-address/actuator";
    const char SUBSCRIBE_CONFIG_TOPIC[] = "ecoprint/mac-address/config";

    const unsigned long MAIN_LOOP_INTERVAL_MS = 10;
    const unsigned long PUBLISH_SENSOR_DATA_INTERVAL_MS = 2000;
    const unsigned long PUBLISH_DEVICE_STATUS_INTERVAL_MS = 1000;
}