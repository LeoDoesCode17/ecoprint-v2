#pragma once
#include "i_page.h"

namespace pages
{
    // Horizontal carousel menu. The number of cards is data-driven - see the
    // `items[]` array in menu_page.cpp. Add or remove entries there; nothing
    // else needs to change.
    class MenuPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        void drawCarousel(TFT_eSPI &tft);
        void drawCard(TFT_eSPI &tft, int itemIndex, int x, bool selected);
        void drawDots(TFT_eSPI &tft);

        int selectedIndex_ = 0;
    };

    extern MenuPage menuPage;
}