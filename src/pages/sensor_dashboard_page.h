#pragma once

#include "i_page.h"

namespace pages
{
    // Read-only dashboard with two buttons: a Lighter toggle (its text
    // switches between "Lighter ON" and "Lighter OFF") and Stop. The
    // encoder moves focus between the buttons and a click activates the
    // focused one. No edit mode is needed because none of the buttons has
    // a value to edit.
    class SensorDashboardPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        enum class Field
        {
            TimerCountdown,
            SetTemperature,
            WaterTemperature,
            AirTemperature,
            AirHumidity,
            ValveStatus,
            PumpStatus,
            LighterStatus,
            Count // not a real field - used only to size arrays / loops
        };

        enum class Button
        {
            Lighter, // toggle: text shows "Lighter ON" / "Lighter OFF"
            Stop,
            Count // not a real button - used only to size loops
        };

        void drawRow(TFT_eSPI &tft, Field field, bool force);
        void formatValue(Field field, char *buffer, size_t bufferSize) const;
        static const char *labelFor(Field field);
        static int rowY(Field field);

        void drawButtons(TFT_eSPI &tft);
        void drawButton(TFT_eSPI &tft, Button button);
        static const char *buttonLabel(Button button);

        unsigned long _countdown_end_ms = 0;
        unsigned long _last_refresh_ms = 0;

        // Index of the focused Button. Defaults to Stop so a plain click on
        // entering the page behaves exactly like before.
        int _focus = static_cast<int>(Button::Stop);

        // Lighter state the toggle button was last drawn with, so its text
        // is only redrawn when the state actually changes.
        bool _lighter_drawn_on = false;

        // Caches the last text drawn for each row, so a row is only
        // redrawn when its value actually changed (avoids flicker across
        // seven rows redrawing every tick for no reason).
        char _last_text[static_cast<int>(Field::Count)][32];
    };

    extern SensorDashboardPage sensor_dashboard_page;
}