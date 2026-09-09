#include "sensor_dashboard_page.h"
#include "page_id.h"
#include <Arduino.h>
#include <string.h>
#include "managers/page_manager.h"
#include "managers/sensor_manager.h"
#include "managers/actuator_manager.h"
#include "managers/network_manager.h"

namespace pages
{
    SensorDashboardPage sensor_dashboard_page;

    namespace
    {
        const int title_y = 18;

        const int first_row_y = 50;
        const int row_height = 30;
        const int row_text_h = 24;

        const int back_button_y = 270;
        const int back_button_height = 40;
        const int back_button_margin_x = 100;

        const unsigned long refresh_interval_ms = 500;
    }

    void SensorDashboardPage::onEnter(TFT_eSPI &tft)
    {
        int timerMinutes = page_manager::get_timer_minutes();
        _countdown_end_ms = millis() + static_cast<unsigned long>(timerMinutes) * 60000UL;
        _last_refresh_ms = 0;

        for (int i = 0; i < static_cast<int>(Field::Count); i++)
        {
            _last_text[i][0] = '\0';
        }

        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("SENSOR DASHBOARD", tft.width() / 2, title_y);

        for (int i = 0; i < static_cast<int>(Field::Count); i++)
        {
            drawRow(tft, static_cast<Field>(i), true);
        }

        tft.fillRoundRect(back_button_margin_x, back_button_y, tft.width() - 2 * back_button_margin_x, back_button_height, 8, TFT_RED);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.setTextFont(4);
        tft.drawString("Back", tft.width() / 2, back_button_y + back_button_height / 2);
    }

    void SensorDashboardPage::onExit()
    {
    }

    void SensorDashboardPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        // No focus to move, nothing to edit - the whole screen is read-only
        // except Back, so any click just leaves.
        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::Menu);
            return;
        }

        unsigned long now = millis();
        if (now - _last_refresh_ms < refresh_interval_ms)
        {
            return;
        }
        _last_refresh_ms = now;

        for (int i = 0; i < static_cast<int>(Field::Count); i++)
        {
            drawRow(tft, static_cast<Field>(i), false);
        }
    }

    void SensorDashboardPage::drawRow(TFT_eSPI &tft, Field field, bool force)
    {
        char text[32];
        formatValue(field, text, sizeof(text));

        int index = static_cast<int>(field);
        if (!force && strcmp(text, _last_text[index]) == 0)
        {
            return; // value hasn't changed - skip the redraw to avoid flicker
        }
        strncpy(_last_text[index], text, sizeof(_last_text[index]) - 1);
        _last_text[index][sizeof(_last_text[index]) - 1] = '\0';

        char line[48];
        snprintf(line, sizeof(line), "%s: %s", labelFor(field), text);

        int y = rowY(field);
        tft.fillRect(0, y, tft.width(), row_text_h, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(2);
        tft.drawString(line, tft.width() / 2, y + row_text_h / 2);
    }

    void SensorDashboardPage::formatValue(Field field, char *buffer, size_t bufferSize) const
    {
        switch (field)
        {
        case Field::TimerCountdown:
        {
            unsigned long nowMs = millis();
            unsigned long remainingMs = (_countdown_end_ms > nowMs) ? (_countdown_end_ms - nowMs) : 0;
            unsigned long remainingSec = remainingMs / 1000UL;
            snprintf(buffer, bufferSize, "%02lu:%02lu", remainingSec / 60UL, remainingSec % 60UL);
            break;
        }
        case Field::SetTemperature:
            snprintf(buffer, bufferSize, "%d C", page_manager::get_temperature_celcius());
            break;
        case Field::WaterTemperature:
            snprintf(buffer, bufferSize, "%.1f C", 20.0);
            break;
        case Field::AirTemperature:
            snprintf(buffer, bufferSize, "%.1f C", 20.0);
            break;
        case Field::AirHumidity:
            snprintf(buffer, bufferSize, "%.0f %%", 98.0);
            break;
        case Field::ValveStatus:
            snprintf(buffer, bufferSize, "%s", true ? "Big" : "Little");
            break;
        case Field::PumpStatus:
            snprintf(buffer, bufferSize, "%s", true ? "On" : "Off");
            break;
        default:
            buffer[0] = '\0';
            break;
        }
    }

    const char *SensorDashboardPage::labelFor(Field field)
    {
        switch (field)
        {
        case Field::TimerCountdown:
            return "Time Left";
        case Field::SetTemperature:
            return "Set Temp";
        case Field::WaterTemperature:
            return "Water Temp";
        case Field::AirTemperature:
            return "Air Temp";
        case Field::AirHumidity:
            return "Air Humidity";
        case Field::ValveStatus:
            return "Valve";
        case Field::PumpStatus:
            return "Pump";
        default:
            return "";
        }
    }

    int SensorDashboardPage::rowY(Field field)
    {
        return first_row_y + static_cast<int>(field) * row_height;
    }
}