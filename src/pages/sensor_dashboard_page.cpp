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

        unsigned long LAST_REFRESH = millis();
        const unsigned long REFRESH_INTERVAL_MS = 500;

        const unsigned long SENSOR_UPDATE_INTERVAL_MS = 1000;
        unsigned long LAST_SENSOR_UPDATE = millis();

        const unsigned long ACTUATOR_UPDATE_INTERVAL_MS = 1000;
        unsigned long LAST_ACTUATOR_UPDATE = millis();

        const unsigned long PUBLISH_SENSOR_DATA_INTERVAL_MS = 1000;
        unsigned long LAST_PUBLISH_SENSOR_DATA = millis();

        const unsigned long PUBLISH_ACTUATOR_DATA_INTERVAL_MS = 1000;
        unsigned long LAST_PUBLISH_ACTUATOR_DATA = millis();

        const unsigned long CONTROL_INTERVAL_MS = 100;
        unsigned long LAST_CONTROL = millis();

        ecoprint_sensor_t sensor_data;
        ecoprint_actuator_t actuator_data;
        int timer;

    }

    void SensorDashboardPage::onEnter(TFT_eSPI &tft)
    {
        // init the struct variables
        sensor_data.water_temperature = sensor_manager::smoothed_thermocouple_temperature_celcius(0);
        sensor_data.air_temperature = sensor_manager::sht3x_temperature_celcius();
        sensor_data.humidity = sensor_manager::sht3x_humidity_percent();
        sensor_data.is_fire_on = true;
        sensor_data.is_water_sufficient = true;
        sensor_data.setpoint = sensor_manager::get_setpoint_temperature();

        actuator_data.is_lighter_on = actuator_manager::is_lighter_on();
        actuator_data.is_max_valve_opening = true;
        actuator_data.is_pump_on = actuator_manager::is_pump_on();
        actuator_data.is_valve_open = actuator_manager::is_valve_open();
        actuator_data.setpoint = sensor_manager::get_setpoint_temperature();
        actuator_data.valve_degree = 10;

        timer = sensor_manager::get_timer();

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
        // Put the update sensor, publish message, and control here

        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::Menu);
            return;
        }

        unsigned long now = millis();
        if (now - _last_refresh_ms >= REFRESH_INTERVAL_MS)
        {
            _last_refresh_ms = now;

            for (int i = 0; i < static_cast<int>(Field::Count); i++)
            {
                drawRow(tft, static_cast<Field>(i), false);
            }
        }

        if (millis() - LAST_SENSOR_UPDATE >= SENSOR_UPDATE_INTERVAL_MS)
        {
            LAST_SENSOR_UPDATE = millis();

            sensor_data.water_temperature = sensor_manager::smoothed_thermocouple_temperature_celcius(0);
            sensor_data.air_temperature = sensor_manager::sht3x_temperature_celcius();
            sensor_data.humidity = sensor_manager::sht3x_humidity_percent();
            sensor_data.is_fire_on = true;
            sensor_data.is_water_sufficient = true;
            sensor_data.setpoint = sensor_manager::get_setpoint_temperature();
        }

        if (millis() - LAST_ACTUATOR_UPDATE >= ACTUATOR_UPDATE_INTERVAL_MS)
        {
            LAST_ACTUATOR_UPDATE = millis();

            actuator_data.is_lighter_on = actuator_manager::is_lighter_on();
            actuator_data.is_max_valve_opening = true;
            actuator_data.is_pump_on = actuator_manager::is_pump_on();
            actuator_data.is_valve_open = actuator_manager::is_valve_open();
            actuator_data.setpoint = sensor_manager::get_setpoint_temperature();
            actuator_data.valve_degree = 10;
        }

        if (millis() - LAST_PUBLISH_ACTUATOR_DATA >= PUBLISH_ACTUATOR_DATA_INTERVAL_MS)
        {
            LAST_PUBLISH_ACTUATOR_DATA = millis();

            // publish actuator data
        }

        if (millis() - LAST_PUBLISH_SENSOR_DATA >= PUBLISH_SENSOR_DATA_INTERVAL_MS)
        {
            LAST_PUBLISH_SENSOR_DATA = millis();

            // publish sensor data
        }

        if (millis() - LAST_CONTROL >= CONTROL_INTERVAL_MS)
        {
            LAST_CONTROL = millis();

            // control temperature
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
            snprintf(buffer, bufferSize, "%.1f C", sensor_data.setpoint);
            break;
        case Field::WaterTemperature:
            snprintf(buffer, bufferSize, "%.1f C", sensor_data.water_temperature);
            break;
        case Field::AirTemperature:
            snprintf(buffer, bufferSize, "%.1f C", sensor_data.air_temperature);
            break;
        case Field::AirHumidity:
            snprintf(buffer, bufferSize, "%.0f %%", sensor_data.humidity);
            break;
        case Field::ValveStatus:
            snprintf(buffer, bufferSize, "%s", actuator_manager::is_valve_open() ? "Big" : "Little");
            break;
        case Field::PumpStatus:
            snprintf(buffer, bufferSize, "%s", actuator_manager::is_pump_on() ? "On" : "Off");
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