#pragma once

#include "i_page.h"
#include "config/type.h"

namespace pages
{
    // Read-only dashboard: a state machine indicator under the title (shows
    // the state's name, e.g. FILLING_WATER), a grid of stat cards (timer,
    // temperatures, humidity, valve, pump, lighter), plus two buttons: a Lighter toggle
    // (its text switches between "Lighter ON" and "Lighter OFF") and Stop.
    // The encoder moves focus between the buttons and a click activates the
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

        // State machine indicator: redrawn only when the state changes
        void refreshState(TFT_eSPI &tft);
        void drawState(TFT_eSPI &tft, StateMachine state);
        static const char *stateName(StateMachine state);

        void drawStat(TFT_eSPI &tft, Field field, bool force);
        void formatValue(Field field, char *buffer, size_t bufferSize) const;
        static const char *labelFor(Field field);
        static uint16_t valueColorFor(Field field, const char *text);

        void drawButtons(TFT_eSPI &tft);
        void drawButton(TFT_eSPI &tft, Button button);
        static const char *buttonLabel(Button button);

        unsigned long _countdown_end_ms = 0;
        unsigned long _last_refresh_ms = 0;

        // State machine value the indicator was last drawn with
        // (-1 = nothing drawn yet)
        int _last_state = -1;

        // Index of the focused Button. Defaults to Stop so a plain click on
        // entering the page behaves exactly like before.
        int _focus = static_cast<int>(Button::Stop);

        // Lighter state the toggle button was last drawn with, so its text
        // is only redrawn when the state actually changes.
        bool _lighter_drawn_on = false;

        // Caches the last text drawn for each stat card, so a card is only
        // redrawn when its value actually changed (avoids flicker across
        // eight cards redrawing every tick for no reason).
        char _last_text[static_cast<int>(Field::Count)][32];
    };

    extern SensorDashboardPage sensor_dashboard_page;
}