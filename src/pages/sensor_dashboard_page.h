#pragma once

#include "i_page.h"

namespace pages
{
    // This page has only one action (Back) and nothing to edit, so unlike
    // ModifyServo/CollectData it does NOT need a Focus/edit-mode state
    // machine - same idea as Page1/Page3: any click just leaves.
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
            Count // not a real field - used only to size arrays / loops
        };

        void drawRow(TFT_eSPI &tft, Field field, bool force);
        void formatValue(Field field, char *buffer, size_t bufferSize) const;
        static const char *labelFor(Field field);
        static int rowY(Field field);

        unsigned long _countdown_end_ms = 0;
        unsigned long _last_refresh_ms = 0;

        // Caches the last text drawn for each row, so a row is only
        // redrawn when its value actually changed (avoids flicker across
        // seven rows redrawing every tick for no reason).
        char _last_text[static_cast<int>(Field::Count)][32];
    };

    extern SensorDashboardPage sensor_dashboard_page;
}