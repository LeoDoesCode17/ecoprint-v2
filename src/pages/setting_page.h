#pragma once
#include "i_page.h"

namespace pages
{
    class SettingPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        void drawButton(TFT_eSPI &tft, int index);
        int _selectedIndex = 0;
    };

    extern SettingPage settingPage;
}