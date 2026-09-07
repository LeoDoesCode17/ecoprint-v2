#pragma once

#include "i_page.h"

namespace pages
{
    class ModifyServoPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        // Which of the three controls is currently highlighted.
        enum class Focus
        {
            Slider,
            OpenButton,
            BackButton
        };

        void drawControl(TFT_eSPI &tft, Focus which);
        void drawSlider(TFT_eSPI &tft);
        void drawButton(TFT_eSPI &tft, Focus which);
        void openServo(TFT_eSPI &tft);

        int _current_servo_degree = 0;
        Focus _focus = Focus::Slider;
        bool _is_editing_slider = false;
        unsigned long _open_flash_until_ms = 0;
    };

    extern ModifyServoPage modify_servo_page;
}