#include "modify_servo_page.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include "managers/actuator_manager.h"
#include <Arduino.h>

namespace pages
{
    ModifyServoPage modify_servo_page;

    namespace
    {
        const int titleY = 50;

        const int sliderLabelY = 100;
        const int sliderTrackY = 130;
        const int sliderTrackH = 18;
        const int sliderMarginX = 40;

        const int openButtonY = 175;
        const int backButtonY = 240;
        const int buttonHeight = 45;
        const int buttonMarginX = 60;

        const int focusCount = 3;
    }

    void ModifyServoPage::onEnter(TFT_eSPI &tft)
    {
        _focus = Focus::Slider;
        _is_editing_slider = false;
        _open_flash_until_ms = 0;
        // _current_servo_degree is intentionally NOT reset here, so the page
        // remembers the last angle you set even if you leave and come back.

        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("MODIFY SERVO", tft.width() / 2, titleY);

        drawControl(tft, Focus::Slider);
        drawControl(tft, Focus::OpenButton);
        drawControl(tft, Focus::BackButton);
    }

    void ModifyServoPage::onExit()
    {
    }

    void ModifyServoPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        // Non-blocking revert of the "Opening..." flash - same millis()-gated
        // pattern as Page1, so a press here never blocks the encoder/button.
        if (_open_flash_until_ms != 0 && millis() >= _open_flash_until_ms)
        {
            _open_flash_until_ms = 0;
            drawButton(tft, Focus::OpenButton);
        }

        if (_is_editing_slider)
        {
            // While editing, rotation changes the value instead of moving focus.
            if (encoderDelta != 0)
            {
                _current_servo_degree += static_cast<int>(encoderDelta);
                _current_servo_degree = constrain(_current_servo_degree, 0, 90);
                drawSlider(tft);
            }

            if (buttonPressed)
            {
                _is_editing_slider = false;
                drawSlider(tft); // redraw with the "focused, not editing" border
            }

            return;
        }

        // Browsing mode: rotation moves focus between slider / open / back.
        if (encoderDelta != 0)
        {
            Focus previous = _focus;

            int next = (static_cast<int>(_focus) + static_cast<int>(encoderDelta)) % focusCount;
            if (next < 0)
            {
                next += focusCount;
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
            case Focus::Slider:
                _is_editing_slider = true;
                drawSlider(tft);
                break;

            case Focus::OpenButton:
                openServo(tft);
                break;

            case Focus::BackButton:
                page_manager::navigateTo(PageId::Menu);
                break;
            }
        }
    }

    void ModifyServoPage::drawControl(TFT_eSPI &tft, Focus which)
    {
        if (which == Focus::Slider)
        {
            drawSlider(tft);
        }
        else
        {
            drawButton(tft, which);
        }
    }

    void ModifyServoPage::drawSlider(TFT_eSPI &tft)
    {
        bool focused = (_focus == Focus::Slider);
        uint16_t borderColor = _is_editing_slider ? TFT_GREEN : (focused ? TFT_BLUE : TFT_DARKGREY);

        char label[24];
        snprintf(label, sizeof(label), "Angle: %d deg", _current_servo_degree);

        tft.fillRect(0, sliderLabelY, tft.width(), 24, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(2);
        tft.drawString(label, tft.width() / 2, sliderLabelY + 12);

        int trackX = sliderMarginX;
        int trackW = tft.width() - 2 * sliderMarginX;

        tft.fillRect(trackX, sliderTrackY, trackW, sliderTrackH, TFT_BLACK);
        tft.drawRect(trackX - 2, sliderTrackY - 2, trackW + 4, sliderTrackH + 4, borderColor);

        int fillW = map(_current_servo_degree, 0, 90, 0, trackW);
        if (fillW > 0)
        {
            tft.fillRect(trackX, sliderTrackY, fillW, sliderTrackH, TFT_BLUE);
        }
    }

    void ModifyServoPage::drawButton(TFT_eSPI &tft, Focus which)
    {
        int y = (which == Focus::OpenButton) ? openButtonY : backButtonY;
        const char *label = (which == Focus::OpenButton) ? "Open Servo" : "Back";
        bool focused = (_focus == which);
        uint16_t fillColor = focused ? (which == Focus::BackButton ? TFT_RED : TFT_BLUE) : TFT_DARKGREY;

        tft.fillRoundRect(buttonMarginX, y, tft.width() - 2 * buttonMarginX, buttonHeight, 8, fillColor);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, fillColor);
        tft.setTextFont(4);
        tft.drawString(label, tft.width() / 2, y + buttonHeight / 2);
    }

    void ModifyServoPage::openServo(TFT_eSPI &tft)
    {
        // TODO: wire this to your actual servo driver, e.g.:
        //   servo_manager::set_angle(_current_servo_degree);
        Serial.printf("Opening servo to %d degrees\n", _current_servo_degree);

        _open_flash_until_ms = millis() + 400;

        tft.fillRoundRect(buttonMarginX, openButtonY, tft.width() - 2 * buttonMarginX, buttonHeight, 8, TFT_GREEN);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_BLACK, TFT_GREEN);
        tft.setTextFont(4);
        tft.drawString("Opening...", tft.width() / 2, openButtonY + buttonHeight / 2);
        actuator_manager::open_valve_by_degree(_current_servo_degree);
    }
}