#include "mqtt.h"
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include "config/constants.h"

namespace
{
    static WiFiClient esp_client;
    static PubSubClient mqtt_client(esp_client);

    static IPAddress resolveMdnsHost(const char *host)
    {
        Serial.print("Resolving mDNS host: ");
        Serial.print(host);
        Serial.println(".local");

        // Perform mDNS query with a 5000ms timeout
        IPAddress serverIp = MDNS.queryHost(host, 5000);

        if (serverIp == INADDR_NONE)
        {
            Serial.println("Error: Could not resolve mDNS host.");
        }
        else
        {
            Serial.print("Successfully resolved to IP: ");
            Serial.println(serverIp);
        }
        return serverIp;
    }
}
namespace mqtt
{
    void initialize()
    {
        if (!MDNS.begin("esp32_mqtt_client"))
        {
            Serial.println("Error setting up MDNS responder!");
            while (1)
            {
                delay(1000);
            }
        }
        Serial.println("mDNS responder started");

        // 3. Resolve Raspberry Pi IP address
        IPAddress brokerIp = resolveMdnsHost(constant::MQTT_BROKER_MDNS);

        // 4. Configure MQTT server with the resolved IP
        if (brokerIp != INADDR_NONE)
        {
            mqtt_client.setServer(brokerIp, constant::MQTT_PORT);
        }
        else
        {
            // if not resolved, fallback to online hive IP or hostname
            mqtt_client.setServer("broker.hivemq.com", 1883);
        }
    }
    void connect_or_reconnect()
    {
        // Loop until we're reconnected
        while (!mqtt_client.connected())
        {
            Serial.print("Attempting MQTT connection...");
            // Attempt to connect
            if (mqtt_client.connect(constant::MQTT_CLIENT_ID))
            {
                Serial.println("connected");
                // Once connected, publish an announcement...
                mqtt_client.publish(constant::PUBLISH_TOPIC, "Hello from ESP32");
                // ... and resubscribe
                mqtt_client.subscribe(constant::SUBSCRIBE_TOPIC);
                mqtt_client.subscribe(constant::SUBSCRIBE_CONFIG_TOPIC);
                mqtt_client.subscribe(constant::SUBSCRIBE_ACTUATOR_TOPIC);
            }
            else
            {
                Serial.print("failed, rc=");
                Serial.print(mqtt_client.state());
                Serial.println(" try again in 5 seconds");
                // Wait 5 seconds before retrying
                delay(5000);
            }
        }
    }
    void loop()
    {
        mqtt_client.loop();
    }
    bool is_connected()
    {
        return mqtt_client.connected();
    }
    void set_callback(mqtt_callback callback) {
        mqtt_client.setCallback(callback);
    }
    bool publish_message(const char *topic, const char *payload)
    {
        return mqtt_client.publish(topic, payload);
    }
}