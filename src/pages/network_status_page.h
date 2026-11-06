#pragma once
#include "i_page.h"
#include "page1.h"

namespace pages
{
    class NetworkStatusPage : public IPage
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

    extern NetworkStatusPage networkStatusPage;
}