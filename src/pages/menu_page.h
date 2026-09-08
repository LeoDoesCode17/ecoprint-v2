#pragma once
#include "i_page.h"

namespace pages
{
    // The number of buttons is entirely data-driven - see the `items[]`
    // array in menu_page.cpp. Add or remove entries there to add or
    // remove buttons; nothing else needs to change.
    class MenuPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        void drawButton(TFT_eSPI &tft, int index);
        int selectedIndex_ = 0;
    };

    extern MenuPage menuPage;
}
