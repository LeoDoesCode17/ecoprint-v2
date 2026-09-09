#include "collect_data_page.h"
#include "page_id.h"
#include <Arduino.h>
#include "managers/page_manager.h"
#include "managers/sensor_manager.h"

namespace pages
{
    namespace
    {
        const int title_y = 25;

        // Each slider only needs to define where its own block starts
        // (anchor_y). The track is always drawn a fixed offset below the
        // label, so adding a third slider later only means adding one more
        // anchor_y constant, not more layout math.
        const int temperature_anchor_y = 55;
        const int timer_anchor_y = 130;
        const int slider_track_offset = 30; // track_y = anchor_y + this
        const int slider_label_h = 24;
        const int slider_track_h = 18;
        const int slider_margin_x = 40;

        const int start_button_y = 205;
        const int back_button_y = 260;
        const int button_height = 40;
        const int button_margin_x = 60;

        const int focus_count = 4;
    }

    CollectDataPage collect_data_page;

    void CollectDataPage::onEnter(TFT_eSPI &tft)
    {
        _focus = Focus::TemperatureSlider;
        _is_editing = false;
        _current_temparture_celcius_value = 0;
        _current_timer_minute_value = 0;

        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("COLLECT DATA", tft.width() / 2, title_y);

        drawControl(tft, Focus::TemperatureSlider);
        drawControl(tft, Focus::TimerSlider);
        drawControl(tft, Focus::StartButton);
        drawControl(tft, Focus::BackButton);
    }

    void CollectDataPage::onExit()
    {
        // Set private properties to default value
        _is_editing = false;
        _current_temparture_celcius_value = 0;
        _current_timer_minute_value = 0;
    }

    void CollectDataPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (_is_editing)
        {
            SliderType type = (_focus == Focus::TemperatureSlider) ? SliderType::Temperature : SliderType::Timer;

            if (encoderDelta != 0)
            {
                int &value = valueFor(type);
                value += static_cast<int>(encoderDelta);
                value = constrain(value, minFor(type), maxFor(type));
                drawSlider(tft, type);
            }

            if (buttonPressed)
            {
                _is_editing = false;
                drawSlider(tft, type);
            }

            return;
        }

        if (encoderDelta != 0)
        {
            Focus previous = _focus;

            int next = (static_cast<int>(_focus) + static_cast<int>(encoderDelta)) % focus_count;
            if (next < 0)
            {
                next += focus_count;
            }
            _focus = static_cast<Focus>(next);

            if (_focus != previous)
            {
                drawControl(tft, previous);
                drawControl(tft, _focus);
            }
        }

        if (buttonPressed)
        {
            switch (_focus)
            {
            case Focus::TemperatureSlider:
            case Focus::TimerSlider:
                _is_editing = true;
                drawControl(tft, _focus);
                break;

            case Focus::StartButton:
                startCollection();
                break;

            case Focus::BackButton:
                page_manager::navigateTo(PageId::Menu);
                break;
            }
        }
    }

    void CollectDataPage::drawControl(TFT_eSPI &tft, Focus which)
    {
        if (which == Focus::TemperatureSlider)
        {
            drawSlider(tft, SliderType::Temperature);
        }
        else if (which == Focus::TimerSlider)
        {
            drawSlider(tft, SliderType::Timer);
        }
        else
        {
            drawButton(tft, which);
        }
    }

    void CollectDataPage::drawButton(TFT_eSPI &tft, Focus which)
    {
        const int y = (which == Focus::StartButton) ? start_button_y : back_button_y;
        const char *label = (which == Focus::StartButton) ? "Start" : "Back";
        const bool is_focused = (_focus == which);
        uint16_t fill_color = is_focused ? (which == Focus::BackButton ? TFT_RED : TFT_BLUE) : TFT_DARKGREY;

        tft.fillRoundRect(button_margin_x, y, tft.width() - 2 * button_margin_x, button_height, 8, fill_color);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, fill_color);
        tft.setTextFont(4);
        tft.drawString(label, tft.width() / 2, y + button_height / 2);
    }

    void CollectDataPage::drawSlider(TFT_eSPI &tft, SliderType type)
    {
        const bool is_focused = (_focus == focusFor(type));
        const bool is_editing = is_focused && _is_editing;
        uint16_t border_color = is_editing ? TFT_GREEN : (is_focused ? TFT_BLUE : TFT_DARKGREY);

        const int anchor_y = anchorYFor(type);
        const int label_y = anchor_y;
        const int track_y = anchor_y + slider_track_offset;

        char label_buffer[40];
        labelFor(type, label_buffer, sizeof(label_buffer));

        tft.fillRect(0, label_y, tft.width(), slider_label_h, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(2);
        tft.drawString(label_buffer, tft.width() / 2, label_y + slider_label_h / 2);

        const int track_x = slider_margin_x;
        const int track_w = tft.width() - 2 * slider_margin_x;

        tft.fillRect(track_x, track_y, track_w, slider_track_h, TFT_BLACK);
        tft.drawRect(track_x - 2, track_y - 2, track_w + 4, slider_track_h + 4, border_color);

        int fill_w = map(valueFor(type), minFor(type), maxFor(type), 0, track_w);
        if (fill_w > 0)
        {
            tft.fillRect(track_x, track_y, fill_w, slider_track_h, TFT_BLUE);
        }
    }

    void CollectDataPage::startCollection()
    {
        // Hand the configured values off to page_manager so the Sensor
        // Dashboard (or anything else) can read them after we navigate away -
        // this page's own members disappear the moment we leave it.
        sensor_manager::set_setpoint_temperature((float)_current_temparture_celcius_value);
        sensor_manager::set_timer(_current_timer_minute_value);
        page_manager::navigateTo(PageId::SensorDashboard);
    }

    int &CollectDataPage::valueFor(SliderType type)
    {
        return (type == SliderType::Temperature) ? _current_temparture_celcius_value : _current_timer_minute_value;
    }

    int CollectDataPage::minFor(SliderType type) const
    {
        return (type == SliderType::Temperature) ? 50 : 30;
    }

    int CollectDataPage::maxFor(SliderType type) const
    {
        return (type == SliderType::Temperature) ? 100 : 120;
    }

    int CollectDataPage::anchorYFor(SliderType type) const
    {
        return (type == SliderType::Temperature) ? temperature_anchor_y : timer_anchor_y;
    }

    void CollectDataPage::labelFor(SliderType type, char *buffer, size_t bufferSize) const
    {
        if (type == SliderType::Temperature)
        {
            snprintf(buffer, bufferSize, "Temperature: %d Celcius", _current_temparture_celcius_value);
        }
        else
        {
            snprintf(buffer, bufferSize, "Timer: %d minutes", _current_timer_minute_value);
        }
    }

    CollectDataPage::Focus CollectDataPage::focusFor(SliderType type)
    {
        return (type == SliderType::Temperature) ? Focus::TemperatureSlider : Focus::TimerSlider;
    }
}