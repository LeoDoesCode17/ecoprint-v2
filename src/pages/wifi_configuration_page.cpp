#include "wifi_configuration_page.h"
#include <string.h>
#include "page_id.h"
#include "managers/network_manager.h"

namespace pages
{
    namespace
    {
        const char *validateSsid(const char *text)
        {
            return (text[0] != '\0') ? nullptr : "SSID cannot be empty";
        }

        // WPA2 passwords are 8-63 characters; empty is allowed for open networks
        const char *validatePassword(const char *text)
        {
            const size_t len = strlen(text);
            return (len == 0 || len >= 8) ? nullptr : "Password must be 8-63 characters (or empty)";
        }

        // Add/remove entries here to change the cards on this page; nothing
        // else needs to change. Max lengths match the buffers in network_manager.
        const CredentialField fields[] = {
            // label, masked, numericOnly, maxLength, get, set, validate
            {"SSID", false, false, 32, network_manager::get_wifi_ssid, network_manager::set_wifi_ssid, validateSsid},
            {"Password", true, false, 63, network_manager::get_wifi_password, network_manager::set_wifi_password, validatePassword},
        };
    }

    WifiConfigurationPage::WifiConfigurationPage()
        : CredentialPage("Wifi Configuration", fields,
                         static_cast<int>(sizeof(fields) / sizeof(fields[0])),
                         PageId::NetworkConfiguration)
    {
    }

    WifiConfigurationPage wifiConfigurationPage;
}