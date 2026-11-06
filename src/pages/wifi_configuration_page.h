#pragma once
#include "credential_page.h"

namespace pages
{
    // WiFi credentials: SSID and password. All behavior lives in
    // CredentialPage; this class only supplies the fields (see the .cpp).
    class WifiConfigurationPage : public CredentialPage
    {
    public:
        WifiConfigurationPage();
    };

    extern WifiConfigurationPage wifiConfigurationPage;
}