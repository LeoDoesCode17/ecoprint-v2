#include "wifi.h"
#include <WiFi.h>
#include "config/constants.h"
#include "esp_mac.h"

namespace wifi
{
    void initialize(const char *ssid, const char *password)
    {
        // We start by connecting to a WiFi network
        Serial.println();
        Serial.print("[WiFi] Connecting to ");
        Serial.println(ssid);

        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
    }

    void connect_or_reconnect()
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            return;
        }

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

    bool is_connected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

}