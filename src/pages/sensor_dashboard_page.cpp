#include "sensor_dashboard_page.h"
#include "page_id.h"
#include <Arduino.h>
#include <string.h>
#include "managers/page_manager.h"
#include "managers/sensor_manager.h"
#include "managers/actuator_manager.h"
#include "managers/network_manager.h"
#include "managers/queue_manager.h"
#include "config/constants.h"
#include "theme.h"

namespace pages
{
    SensorDashboardPage sensor_dashboard_page;
    namespace t = theme;

    namespace
    {
        // Layout (designed for 480x320) - theme and card style shared with
        // the other pages (see theme.h)
        constexpr int kTitleY = 22;
        constexpr int kDividerY = 44;
        constexpr int kCardRadius = 12;

        // Stat cards: a kStatCols x kStatRows grid of read-only cards
        constexpr int kStatCols = 4;
        constexpr int kStatRows = 2;
        constexpr int kGridMarginX = 16;
        constexpr int kGridTop = 58;
        constexpr int kStatH = 84;
        constexpr int kStatGap = 8;

        // Button row: [Lighter ON/OFF toggle] [Stop]
        constexpr int kBtnY = 246;
        constexpr int kBtnH = 42;
        constexpr int kBtnGap = 12;

        constexpr int kHintY = 306;

        // Color for "active" values (same green as edit mode / ON pills)
        constexpr uint16_t kActive = t::rgb(110, 200, 100);

        // Position and width of the stat card for a given Field index
        void statRect(TFT_eSPI &tft, int index, int &x, int &y, int &w)
        {
            w = (tft.width() - 2 * kGridMarginX - (kStatCols - 1) * kStatGap) / kStatCols;
            x = kGridMarginX + (index % kStatCols) * (w + kStatGap);
            y = kGridTop + (index / kStatCols) * (kStatH + kStatGap);
        }

        unsigned long LAST_UPDATE_DEVICE_STATUS = millis();
        const unsigned long UPDATE_DEVICE_STATUS_INTERVAL_MS = 1990;

        unsigned long LAST_REFRESH = millis();
        const unsigned long REFRESH_INTERVAL_MS = 500;

        const unsigned long SENSOR_UPDATE_INTERVAL_MS = 1990;
        unsigned long LAST_SENSOR_UPDATE = millis();

        const unsigned long ACTUATOR_UPDATE_INTERVAL_MS = 90;
        unsigned long LAST_ACTUATOR_UPDATE = millis();

        const unsigned long PUBLISH_SENSOR_DATA_INTERVAL_MS = 1990;
        unsigned long LAST_PUBLISH_SENSOR_DATA = millis();

        const unsigned long CONTROL_INTERVAL_MS = 90;
        unsigned long LAST_CONTROL = millis();

        const unsigned long UPDATE_TIMER_INTERVAL_MS = 990;
        unsigned long LAST_UPDATE_TIMER = millis();

        ecoprint_sensor_t sensor_data;
        ecoprint_actuator_t previous_actuator_data, actuator_data;

        sensor_t _outbound_sensor_data;
        actuator_t _outbound_actuator_data;
        unsigned long timer_second;

        bool is_steaming = false;

    }

    void SensorDashboardPage::onEnter(TFT_eSPI &tft)
    {
        // init the struct variables
        sensor_data.water_temperature = sensor_manager::smoothed_thermocouple_temperature_celcius(0);
        sensor_data.air_temperature = sensor_manager::sht3x_temperature_celcius();
        sensor_data.humidity = sensor_manager::sht3x_humidity_percent();
        sensor_data.is_fire_on = true;
        sensor_data.is_water_sufficient = true;
        sensor_data.setpoint = static_cast<int>(sensor_manager::get_setpoint_temperature());

        _outbound_sensor_data.water_temperature = sensor_data.water_temperature;
        _outbound_sensor_data.air_temperature = sensor_data.air_temperature;
        _outbound_sensor_data.humidity = sensor_data.humidity;
        _outbound_sensor_data.is_fire_on = sensor_data.water_temperature;
        _outbound_sensor_data.is_water_sufficient = sensor_data.water_temperature;

        actuator_data.is_lighter_on = actuator_manager::is_lighter_on();
        actuator_data.is_max_valve_opening = actuator_manager::is_valve_max_opening();
        actuator_data.is_pump_on = actuator_manager::is_pump_on();
        actuator_data.is_valve_open = actuator_manager::is_valve_open();
        actuator_data.setpoint = static_cast<int>(sensor_manager::get_setpoint_temperature());
        actuator_data.valve_degree = actuator_manager::get_current_valve_degree();

        _outbound_actuator_data.is_lighter_on = actuator_data.is_lighter_on;
        _outbound_actuator_data.is_max_valve_opening = actuator_data.is_max_valve_opening;
        _outbound_actuator_data.is_pump_on = actuator_data.is_pump_on;
        _outbound_actuator_data.is_valve_open = actuator_data.is_valve_open;
        _outbound_actuator_data.valve_degree = actuator_data.valve_degree;

        timer_second = sensor_manager::get_timer() * 60;

        int timerMinutes = sensor_manager::get_timer();
        _countdown_end_ms = millis() + static_cast<unsigned long>(timerMinutes) * 60000UL;
        _last_refresh_ms = 0;
        _focus = static_cast<int>(Button::Stop);

        for (int i = 0; i < static_cast<int>(Field::Count); i++)
        {
            _last_text[i][0] = '\0';
        }

        static_assert(static_cast<int>(Field::Count) <= kStatCols * kStatRows,
                      "Not enough room in the stat grid for every Field");

        const int cx = tft.width() / 2;

        tft.fillScreen(t::kBg);

        // Title + divider (same style as the other pages)
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBg);
        tft.drawString("Sensor Dashboard", cx, kTitleY);

        tft.drawFastHLine(cx - 70, kDividerY, 60, t::kSage);
        tft.drawFastHLine(cx + 10, kDividerY, 60, t::kSage);
        tft.fillCircle(cx, kDividerY, 3, t::kOchre);

        for (int i = 0; i < static_cast<int>(Field::Count); i++)
        {
            drawStat(tft, static_cast<Field>(i), true);
        }

        drawButtons(tft);

        // Static hint
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextColor(t::kSage, t::kBg);
        tft.drawString("Turn to switch   |   Press to select", cx, kHintY);
    }

    void SensorDashboardPage::onExit()
    {
        actuator_manager::turn_off_pump();
        actuator_manager::close_valve();
        // The lighter can now be switched on by hand from this page, so make
        // sure it is never left on after leaving the page.
        actuator_manager::turn_off_lighter();
    }

    void SensorDashboardPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        // The encoder moves focus between the buttons; a click activates the
        // focused one (Lighter toggle / Stop).
        // Put the update sensor, publish message, and control here

        // only publish data sensors and actuator when state_machine is not idle
        // ketika waktu sudah selesai maka publish stop command

        if (encoderDelta != 0)
        {
            const int count = static_cast<int>(Button::Count);
            const int previous = _focus;
            _focus = ((_focus + static_cast<int>(encoderDelta)) % count + count) % count;

            if (_focus != previous)
            {
                // Only the two buttons whose focus changed need a redraw
                drawButton(tft, static_cast<Button>(previous));
                drawButton(tft, static_cast<Button>(_focus));
            }
        }

        if (buttonPressed)
        {
            switch (static_cast<Button>(_focus))
            {
            case Button::Lighter:
                // One button toggles the lighter, so no knob rotation is
                // needed to switch between on and off.
                if (actuator_manager::is_lighter_on())
                {
                    Serial.println("[ACTUATOR] Turn off lighter");
                    actuator_manager::turn_off_lighter();
                }
                else
                {
                    Serial.println("[ACTUATOR] Turn on lighter");
                    actuator_manager::turn_on_lighter();
                }
                drawButton(tft, Button::Lighter); // only the text changes
                drawStat(tft, Field::LighterStatus, false);
                break;

            case Button::Stop:
            {
                const command_t stop_command = {.command = 0, .setpoint = static_cast<int>(sensor_manager::get_setpoint_temperature()), .timer_minutes = sensor_manager::get_timer(), .source = command_source_t::LOCAL};
                // network_manager::publish_stop_message(command_data);
                queue_manager::enqueue_command_data(stop_command);
                page_manager::navigateTo(PageId::SteamingSummaryPage);
                return;
            }

            default:
                break;
            }
        }

        unsigned long now = millis();
        if (now - LAST_UPDATE_TIMER >= UPDATE_TIMER_INTERVAL_MS)
        {
            LAST_UPDATE_TIMER = millis();
            if (timer_second > 0)
            {
                timer_second--;
            }
            else
            {
                page_manager::navigateTo(PageId::SteamingSummaryPage);
                // publish stop message to broker
                const command_t stop_command = {.command = 0, .setpoint = static_cast<int>(sensor_manager::get_setpoint_temperature()), .timer_minutes = sensor_manager::get_timer(), .source = command_source_t::LOCAL};
                // network_manager::publish_stop_message(command_data);
                queue_manager::enqueue_command_data(stop_command);
                return; // this page is no longer active - don't draw or control below
            }
        }

        if (now - _last_refresh_ms >= REFRESH_INTERVAL_MS)
        {
            _last_refresh_ms = now;

            for (int i = 0; i < static_cast<int>(Field::Count); i++)
            {
                drawStat(tft, static_cast<Field>(i), false);
            }

            // Keep the button text correct if the lighter changed elsewhere
            if (actuator_manager::is_lighter_on() != _lighter_drawn_on)
            {
                drawButton(tft, Button::Lighter);
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

            _outbound_sensor_data.water_temperature = sensor_data.water_temperature;
            _outbound_sensor_data.air_temperature = sensor_data.air_temperature;
            _outbound_sensor_data.humidity = sensor_data.humidity;
            _outbound_sensor_data.is_fire_on = sensor_data.water_temperature;
            _outbound_sensor_data.is_water_sufficient = sensor_data.water_temperature;

            if (!is_steaming)
            {
                if (sensor_data.water_temperature >= sensor_data.setpoint)
                {
                    is_steaming = true;
                }
            }
            Serial.printf("[SENSOR] Thermocouple offset is: %f\n", sensor_manager::get_thermocouple_offset());
        }

        if (millis() - LAST_ACTUATOR_UPDATE >= ACTUATOR_UPDATE_INTERVAL_MS)
        {
            LAST_ACTUATOR_UPDATE = millis();

            // copy current to previous
            previous_actuator_data.is_lighter_on = actuator_data.is_lighter_on;
            previous_actuator_data.is_max_valve_opening = actuator_data.is_max_valve_opening;
            previous_actuator_data.is_pump_on = actuator_data.is_pump_on;
            previous_actuator_data.is_valve_open = actuator_data.is_valve_open;
            previous_actuator_data.valve_degree = actuator_data.valve_degree;

            // update current data
            actuator_data.is_lighter_on = actuator_manager::is_lighter_on();
            actuator_data.is_max_valve_opening = actuator_manager::is_valve_max_opening();
            actuator_data.is_pump_on = actuator_manager::is_pump_on();
            actuator_data.is_valve_open = actuator_manager::is_valve_open();
            actuator_data.valve_degree = actuator_manager::get_current_valve_degree();

            _outbound_actuator_data.is_lighter_on = actuator_data.is_lighter_on;
            _outbound_actuator_data.is_max_valve_opening = actuator_data.is_max_valve_opening;
            _outbound_actuator_data.is_pump_on = actuator_data.is_pump_on;
            _outbound_actuator_data.is_valve_open = actuator_data.is_valve_open;
            _outbound_actuator_data.valve_degree = actuator_data.valve_degree;

            // compare current and previous
            const bool is_different = (previous_actuator_data.is_lighter_on != actuator_data.is_lighter_on) ||
                                      (previous_actuator_data.is_max_valve_opening != actuator_data.is_max_valve_opening) ||
                                      (previous_actuator_data.is_pump_on != actuator_data.is_pump_on) ||
                                      (previous_actuator_data.is_valve_open != actuator_data.is_valve_open) ||
                                      (previous_actuator_data.valve_degree != actuator_data.valve_degree);

            if (is_different)
            {
                // network_manager::publish_actuator_data(actuator_data);
            }
        }

        if (millis() - LAST_PUBLISH_SENSOR_DATA >= PUBLISH_SENSOR_DATA_INTERVAL_MS)
        {
            LAST_PUBLISH_SENSOR_DATA = millis();
            // network_manager::publish_sensor_data(sensor_data);
            queue_manager::enqueue_sensor_data(_outbound_sensor_data);

            // for data collecting
            // network_manager::publish_actuator_data(actuator_data);
            queue_manager::enqueue_actuator_data(_outbound_actuator_data);
        }

        if (millis() - LAST_CONTROL >= CONTROL_INTERVAL_MS)
        {
            LAST_CONTROL = millis();

            // if (!is_steaming)
            // {
            //     actuator_manager::open_wide_valve();
            // } else {
            //     actuator_manager::open_narrow_valve();
            // }
            const float temp = sensor_data.water_temperature;
            const float upper = sensor_data.setpoint + constant::UPPER_HYSTERESIS_BAND;
            const float lower = sensor_data.setpoint - constant::LOWER_HYSTERESIS_BAND;

            if (temp >= upper)
            {
                actuator_manager::open_narrow_valve();
                actuator_manager::turn_on_pump();
            }
            else if (temp >= sensor_data.setpoint)
            {
                actuator_manager::open_narrow_valve();
            }
            else if (temp <= lower)
            {
                actuator_manager::turn_off_pump();
                actuator_manager::open_wide_valve();
            }
        }
    }

    void SensorDashboardPage::drawStat(TFT_eSPI &tft, Field field, bool force)
    {
        char text[32];
        formatValue(field, text, sizeof(text));

        const int index = static_cast<int>(field);
        if (!force && strcmp(text, _last_text[index]) == 0)
        {
            return; // value hasn't changed - skip the redraw to avoid flicker
        }
        strncpy(_last_text[index], text, sizeof(_last_text[index]) - 1);
        _last_text[index][sizeof(_last_text[index]) - 1] = '\0';

        int x, y, w;
        statRect(tft, index, x, y, w);
        const int cx = x + w / 2;

        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(1);

        if (force)
        {
            // Card frame and label never change, so they are drawn only once
            tft.fillRoundRect(x, y, w, kStatH, kCardRadius, t::kCardIdle);
            tft.drawRoundRect(x, y, w, kStatH, kCardRadius, t::kLeafDark);

            tft.setTextFont(2);
            tft.setTextColor(t::kSage, t::kCardIdle);
            tft.drawString(labelFor(field), cx, y + 18);
        }

        // Value: the text padding paints over the previous value in the card
        // color, so it updates without a clear-then-draw flicker.
        tft.setTextFont(4);
        tft.setTextColor(valueColorFor(field, text), t::kCardIdle);
        tft.setTextPadding(w - 12);
        tft.drawString(text, cx, y + 54);
        tft.setTextPadding(0);
    }

    uint16_t SensorDashboardPage::valueColorFor(Field field, const char *text)
    {
        switch (field)
        {
        case Field::ValveStatus:
            return strcmp(text, "Close") == 0 ? t::kSage : kActive;
        case Field::PumpStatus:
        case Field::LighterStatus:
            return strcmp(text, "On") == 0 ? kActive : t::kSage;
        default:
            return t::kCream;
        }
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
            snprintf(buffer, bufferSize, "%d C", sensor_data.setpoint);
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
            snprintf(buffer, bufferSize, "%s", actuator_manager::is_valve_open() ? (actuator_manager::is_valve_max_opening() ? "Wide" : "Narrow") : "Close");
            break;
        case Field::PumpStatus:
            snprintf(buffer, bufferSize, "%s", actuator_manager::is_pump_on() ? "On" : "Off");
            break;
        case Field::LighterStatus:
            snprintf(buffer, bufferSize, "%s", actuator_manager::is_lighter_on() ? "On" : "Off");
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
            return "Humidity";
        case Field::ValveStatus:
            return "Valve";
        case Field::PumpStatus:
            return "Pump";
        case Field::LighterStatus:
            return "Lighter";
        default:
            return "";
        }
    }

    const char *SensorDashboardPage::buttonLabel(Button button)
    {
        switch (button)
        {
        case Button::Lighter:
            return actuator_manager::is_lighter_on() ? "Lighter ON" : "Lighter OFF";
        case Button::Stop:
            return "Stop";
        default:
            return "";
        }
    }

    void SensorDashboardPage::drawButtons(TFT_eSPI &tft)
    {
        for (int i = 0; i < static_cast<int>(Button::Count); i++)
        {
            drawButton(tft, static_cast<Button>(i));
        }
    }

    void SensorDashboardPage::drawButton(TFT_eSPI &tft, Button button)
    {
        const int count = static_cast<int>(Button::Count);
        const int index = static_cast<int>(button);
        const int w = (tft.width() - 2 * kGridMarginX - (count - 1) * kBtnGap) / count;
        const int x = kGridMarginX + index * (w + kBtnGap);
        const bool focused = (index == _focus);
        const bool isStop = (button == Button::Stop);

        // Same look as the carousel cards: focused = highlighted card with
        // an ochre border (maroon for Stop, like the Back cards).
        const uint16_t fill = focused ? (isStop ? t::kBack : t::kLeafDark) : t::kCardIdle;
        const uint16_t border = focused ? t::kOchre : t::kLeafDark;

        if (button == Button::Lighter)
        {
            _lighter_drawn_on = actuator_manager::is_lighter_on();
        }

        tft.fillRoundRect(x, kBtnY, w, kBtnH, kCardRadius, fill);
        tft.drawRoundRect(x, kBtnY, w, kBtnH, kCardRadius, border);
        if (focused)
        {
            // Thicker border for the focused button
            tft.drawRoundRect(x + 1, kBtnY + 1, w - 2, kBtnH - 2, kCardRadius - 1, border);
        }

        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(1);
        tft.setTextFont(4);
        tft.setTextColor(focused ? t::kCream : t::kSage, fill);
        tft.drawString(buttonLabel(button), x + w / 2, kBtnY + kBtnH / 2);
    }
}