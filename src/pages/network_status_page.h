#pragma once
#include "i_page.h"

namespace pages
{
    // Read-only network overview on a single screen:
    //   - a WiFi panel and an MQTT panel, each with a live Connected /
    //     Disconnected indicator and the current credential values from
    //     network_manager (passwords are masked)
    //   - two buttons: Connect (opens the restart confirmation page, since
    //     the new credentials are applied by restarting the device) and Back.
    //     The encoder moves focus between them, a click activates the
    //     focused one.
    class NetworkStatusPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        enum class Panel
        {
            Wifi,
            Mqtt,
            Count // not a real panel - used only to size loops
        };

        // Frame, title and credential values (drawn once)
        void drawPanel(TFT_eSPI &tft, Panel panel);
        // The Connected / Disconnected indicator (redrawn when it changes)
        void drawStatus(TFT_eSPI &tft, Panel panel, bool connected);
        void refreshStatus(TFT_eSPI &tft);

        enum class Button
        {
            Connect,
            Back,
            Count // not a real button - used only to size loops
        };

        void drawButtons(TFT_eSPI &tft);
        void drawButton(TFT_eSPI &tft, Button button);
        static const char *buttonLabel(Button button);

        // Index of the focused Button. Defaults to Back so a plain click on
        // entering the page just goes back, like before.
        int _focus = static_cast<int>(Button::Back);

        // Last connection state drawn per panel: -1 = nothing drawn yet,
        // 0 = disconnected, 1 = connected. Lets us redraw only on a change.
        int _last_wifi_state = -1;
        int _last_mqtt_state = -1;
        unsigned long _last_refresh_ms = 0;
    };

    extern NetworkStatusPage networkStatusPage;
}