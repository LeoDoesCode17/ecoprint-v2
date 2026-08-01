#include "wifi.h"
#include <WiFi.h>
#include "config/constants.h"

namespace wifi
{
    void connect_or_reconnect()
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            return;
        }
        // We start by connecting to a WiFi network
        Serial.println();
        Serial.print("Connecting to ");
        Serial.println(constant::WIFI_SSID);

        WiFi.mode(WIFI_STA);
        WiFi.begin(constant::WIFI_SSID, constant::WIFI_PASSWORD);

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }

        randomSeed(micros());

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }
}