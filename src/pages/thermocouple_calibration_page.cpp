#include "thermocouple_calibration_page.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include "managers/sensor_manager.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>

namespace pages
{
    ThermocoupleCalibrationPage thermocoupleCalibrationPage;

    namespace
    {
        const int title_y = 14;
        const int live_value_y = 40;

        // Same trick as Collect Data: each slider only needs an anchor_y,
        // the track always sits a fixed offset below the label.
        const int integer_anchor_y = 68;
        const int tenths_anchor_y = 126;
        const int hundredths_anchor_y = 184;
        const int slider_track_offset = 30;
        const int slider_label_h = 24;
        const int slider_track_h = 18;
        const int slider_margin_x = 40;

        const int integer_part_max = 50; // adjust to whatever range your calibration actually needs

        const int buttons_row_y = 248;
        const int button_height = 42;
        const int buttons_margin_x = 20;
        const int buttons_gap = 15;

        const int focus_count = 6;
        const unsigned long live_value_refresh_ms = 500;
    }

    void ThermocoupleCalibrationPage::onEnter(TFT_eSPI &tft)
    {
        _focus = Focus::IntegerSlider;
        _is_editing = false;
        _apply_flash_until_ms = 0;
        _last_live_refresh_ms = 0;
        _last_live_text[0] = '\0';

        // Start from whatever offset is currently applied, rather than
        // always resetting to 0.00 - you want to see what's active now.
        loadOffset(sensor_manager::get_thermocouple_offset());

        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("THERMOCOUPLE CALIBRATION", tft.width() / 2, title_y);

        refreshLiveValueIfDue(tft);

        drawControl(tft, Focus::IntegerSlider);
        drawControl(tft, Focus::TenthsSlider);
        drawControl(tft, Focus::HundredthsSlider);
        drawControl(tft, Focus::SignButton);
        drawControl(tft, Focus::ApplyButton);
        drawControl(tft, Focus::BackButton);
    }

    void ThermocoupleCalibrationPage::onExit()
    {
    }

    void ThermocoupleCalibrationPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        // Live sensor reading refreshes on its own, independent of the
        // encoder/button - same millis()-gated idea as Page1 and the
        // Sensor Dashboard, including only redrawing when it changed.
        refreshLiveValueIfDue(tft);

        // Non-blocking revert of the Apply button's confirmation flash.
        if (_apply_flash_until_ms != 0 && millis() >= _apply_flash_until_ms)
        {
            _apply_flash_until_ms = 0;
            drawButton(tft, Focus::ApplyButton);
        }

        if (_is_editing)
        {
            SliderType type = sliderTypeFor(_focus);

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
            case Focus::IntegerSlider:
            case Focus::TenthsSlider:
            case Focus::HundredthsSlider:
                _is_editing = true;
                drawControl(tft, _focus);
                break;

            case Focus::SignButton:
                _is_positive = !_is_positive;
                drawControl(tft, Focus::SignButton);
                break;

            case Focus::ApplyButton:
                applyOffset(tft);
                break;

            case Focus::BackButton:
                page_manager::navigateTo(PageId::SensorCalibration);
                break;
            }
        }
    }

    void ThermocoupleCalibrationPage::drawControl(TFT_eSPI &tft, Focus which)
    {
        switch (which)
        {
        case Focus::IntegerSlider:
            drawSlider(tft, SliderType::IntegerPart);
            break;
        case Focus::TenthsSlider:
            drawSlider(tft, SliderType::Tenths);
            break;
        case Focus::HundredthsSlider:
            drawSlider(tft, SliderType::Hundredths);
            break;
        default:
            drawButton(tft, which);
            break;
        }
    }

    void ThermocoupleCalibrationPage::drawSlider(TFT_eSPI &tft, SliderType type)
    {
        const bool is_focused = (_focus == focusFor(type));
        const bool is_editing = is_focused && _is_editing;
        uint16_t border_color = is_editing ? TFT_GREEN : (is_focused ? TFT_BLUE : TFT_DARKGREY);

        const int anchor_y = anchorYFor(type);
        const int label_y = anchor_y;
        const int track_y = anchor_y + slider_track_offset;

        char label_buffer[32];
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

    void ThermocoupleCalibrationPage::drawButton(TFT_eSPI &tft, Focus which)
    {
        int x, w;
        buttonGeometry(tft, which, x, w);

        const bool is_focused = (_focus == which);
        uint16_t fill_color = is_focused ? (which == Focus::BackButton ? TFT_RED : TFT_BLUE) : TFT_DARKGREY;

        tft.fillRoundRect(x, buttons_row_y, w, button_height, 8, fill_color);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, fill_color);
        tft.setTextFont(4);
        tft.drawString(buttonLabel(which), x + w / 2, buttons_row_y + button_height / 2);
    }

    void ThermocoupleCalibrationPage::applyOffset(TFT_eSPI &tft)
    {
        float offset = currentOffset();

        // TODO: wire this to your actual calibration storage, e.g.:
        sensor_manager::set_thermocouple_offset(offset);
        Serial.printf("Applying thermocouple offset: %.2f\n", offset);
        sensor_manager::set_thermocouple_offset(offset);

        _apply_flash_until_ms = millis() + 400;

        int x, w;
        buttonGeometry(tft, Focus::ApplyButton, x, w);
        tft.fillRoundRect(x, buttons_row_y, w, button_height, 8, TFT_GREEN);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.setTextFont(4);
        tft.drawString("Apply", x + w / 2, buttons_row_y + button_height / 2);
    }

    void ThermocoupleCalibrationPage::refreshLiveValueIfDue(TFT_eSPI &tft)
    {
        unsigned long now = millis();
        if (now - _last_live_refresh_ms < live_value_refresh_ms)
        {
            return;
        }
        _last_live_refresh_ms = now;

        char value_text[16];
        snprintf(value_text, sizeof(value_text), "%.2f C", sensor_manager::thermocouple_temperature_celcius() + sensor_manager::get_thermocouple_offset());

        if (strcmp(value_text, _last_live_text) == 0)
        {
            return; // unchanged - skip the redraw
        }
        strncpy(_last_live_text, value_text, sizeof(_last_live_text) - 1);
        _last_live_text[sizeof(_last_live_text) - 1] = '\0';

        char line[40];
        snprintf(line, sizeof(line), "Thermocouple: %s", value_text);

        tft.fillRect(0, live_value_y, tft.width(), 24, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(2);
        tft.drawString(line, tft.width() / 2, live_value_y + 12);
    }

    void ThermocoupleCalibrationPage::loadOffset(float offset)
    {
        _is_positive = offset >= 0.0f;
        float magnitude = fabsf(offset);

        int whole = static_cast<int>(magnitude);
        _integer_part = constrain(whole, 0, integer_part_max);

        int hundredths_total = static_cast<int>(roundf((magnitude - whole) * 100.0f));
        if (hundredths_total >= 100)
        {
            hundredths_total = 99;
        }
        _tenths = hundredths_total / 10;
        _hundredths = hundredths_total % 10;
    }

    float ThermocoupleCalibrationPage::currentOffset() const
    {
        float magnitude = _integer_part + _tenths * 0.1f + _hundredths * 0.01f;
        return _is_positive ? magnitude : -magnitude;
    }

    int &ThermocoupleCalibrationPage::valueFor(SliderType type)
    {
        switch (type)
        {
        case SliderType::IntegerPart:
            return _integer_part;
        case SliderType::Tenths:
            return _tenths;
        default:
            return _hundredths;
        }
    }

    int ThermocoupleCalibrationPage::minFor(SliderType type) const
    {
        (void)type;
        return 0; // all three sliders start at 0
    }

    int ThermocoupleCalibrationPage::maxFor(SliderType type) const
    {
        switch (type)
        {
        case SliderType::IntegerPart:
            return integer_part_max;
        default:
            return 9; // tenths and hundredths are single digits
        }
    }

    int ThermocoupleCalibrationPage::anchorYFor(SliderType type) const
    {
        switch (type)
        {
        case SliderType::IntegerPart:
            return integer_anchor_y;
        case SliderType::Tenths:
            return tenths_anchor_y;
        default:
            return hundredths_anchor_y;
        }
    }

    void ThermocoupleCalibrationPage::labelFor(SliderType type, char *buffer, size_t bufferSize) const
    {
        switch (type)
        {
        case SliderType::IntegerPart:
            snprintf(buffer, bufferSize, "Whole number: %d", _integer_part);
            break;
        case SliderType::Tenths:
            snprintf(buffer, bufferSize, "Tenths: %d", _tenths);
            break;
        default:
            snprintf(buffer, bufferSize, "Hundredths: %d", _hundredths);
            break;
        }
    }

    ThermocoupleCalibrationPage::Focus ThermocoupleCalibrationPage::focusFor(SliderType type)
    {
        switch (type)
        {
        case SliderType::IntegerPart:
            return Focus::IntegerSlider;
        case SliderType::Tenths:
            return Focus::TenthsSlider;
        default:
            return Focus::HundredthsSlider;
        }
    }

    ThermocoupleCalibrationPage::SliderType ThermocoupleCalibrationPage::sliderTypeFor(Focus which)
    {
        switch (which)
        {
        case Focus::IntegerSlider:
            return SliderType::IntegerPart;
        case Focus::TenthsSlider:
            return SliderType::Tenths;
        default:
            return SliderType::Hundredths;
        }
    }

    const char *ThermocoupleCalibrationPage::buttonLabel(Focus which) const
    {
        switch (which)
        {
        case Focus::SignButton:
            return _is_positive ? "+" : "-";
        case Focus::ApplyButton:
            return "Apply";
        default:
            return "Back";
        }
    }

    int ThermocoupleCalibrationPage::buttonIndex(Focus which)
    {
        switch (which)
        {
        case Focus::SignButton:
            return 0;
        case Focus::ApplyButton:
            return 1;
        default:
            return 2; // BackButton
        }
    }

    void ThermocoupleCalibrationPage::buttonGeometry(TFT_eSPI &tft, Focus which, int &x, int &w) const
    {
        w = (tft.width() - 2 * buttons_margin_x - 2 * buttons_gap) / 3;
        x = buttons_margin_x + buttonIndex(which) * (w + buttons_gap);
    }
}