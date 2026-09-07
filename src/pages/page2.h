#pragma once
#include "i_page.h"

namespace pages
{
    // Example of a page that reinterprets the encoder for its own purpose
    // (adjusting a value) instead of navigating a list, like the menu does.
    class Page2 : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        void drawValue(TFT_eSPI &tft);
        int value_ = 50;
    };

    extern Page2 page2;
}
