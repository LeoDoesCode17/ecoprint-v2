#include "wifi.h"
#include <WiFi.h>
#include "config/constants.h"
#include "esp_mac.h"

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
    void copy_mac_address(char *out, int len)
    {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        snprintf(out, len, "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
}