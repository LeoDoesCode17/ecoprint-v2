#pragma once
#include "i_page.h"

namespace pages
{
    // Asks the user to confirm before the ESP32 restarts to apply the saved
    // WiFi/MQTT credentials.
    //
    //   Restart -> saves the credentials to flash, puts the actuators in a
    //              safe state and calls ESP.restart()
    //   Cancel  -> back to the network status page
    //
    // The encoder moves focus between the two buttons; Cancel is focused by
    // default so a stray click can never restart the device.
    class RestartConfirmationPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        enum class Button
        {
            Restart,
            Cancel,
            Count // not a real button - used only to size loops
        };

        void drawMessage(TFT_eSPI &tft);
        void drawButtons(TFT_eSPI &tft);
        void drawButton(TFT_eSPI &tft, Button button);
        static const char *buttonLabel(Button button);
        void restartDevice(TFT_eSPI &tft);

        int _focus = static_cast<int>(Button::Cancel);

        // True if a steaming run is in progress when the page opens, so the
        // message can warn that the restart will interrupt it.
        bool _run_active = false;
    };

    extern RestartConfirmationPage restartConfirmationPage;
}