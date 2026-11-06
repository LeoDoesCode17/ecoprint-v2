#pragma once
#include "credential_page.h"

namespace pages
{
    // MQTT credentials: broker host, port, username and password. All
    // behavior lives in CredentialPage; this class only supplies the fields.
    class MqttConfigurationPage : public CredentialPage
    {
    public:
        MqttConfigurationPage();
    };

    extern MqttConfigurationPage mqttConfigurationPage;
}