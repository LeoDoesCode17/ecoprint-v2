#pragma once
#include "i_page.h"

namespace pages
{
    class LandingPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        void drawPrompt(TFT_eSPI &tft, bool visible);

        unsigned long lastBlinkMs_ = 0;
        bool promptVisible_ = true;
    };

    extern LandingPage landing_page;
}