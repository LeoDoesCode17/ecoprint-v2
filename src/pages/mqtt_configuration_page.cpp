#include "mqtt_configuration_page.h"
#include <stdlib.h>
#include "page_id.h"
#include "managers/network_manager.h"

namespace pages
{
    namespace
    {
        const char *validateHost(const char *text)
        {
            return (text[0] != '\0') ? nullptr : "Broker host cannot be empty";
        }

        const char *validatePort(const char *text)
        {
            char *end = nullptr;
            const long port = strtol(text, &end, 10);
            if (text[0] == '\0' || *end != '\0' || port < 1 || port > 65535)
            {
                return "Port must be a number from 1 to 65535";
            }
            return nullptr;
        }

        // Add/remove entries here to change the cards on this page; nothing
        // else needs to change. Max lengths match the buffers in network_manager.
        // Username/password are not validated: brokers without auth use empty values.
        const CredentialField fields[] = {
            // label, masked, numericOnly, maxLength, get, set, validate
            {"Broker Host", false, false, 63, network_manager::get_mqtt_broker_host, network_manager::set_mqtt_broker_host, validateHost},
            {"Port", false, true, 5, network_manager::get_mqtt_port, network_manager::set_mqtt_port, validatePort},
            {"Username", false, false, 63, network_manager::get_mqtt_username, network_manager::set_mqtt_username, nullptr},
            {"Password", true, false, 63, network_manager::get_mqtt_password, network_manager::set_mqtt_password, nullptr},
        };
    }

    MqttConfigurationPage::MqttConfigurationPage()
        : CredentialPage("MQTT Configuration", fields,
                         static_cast<int>(sizeof(fields) / sizeof(fields[0])),
                         PageId::NetworkConfiguration)
    {
    }

    MqttConfigurationPage mqttConfigurationPage;
}