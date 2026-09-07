#pragma once
#include "i_page.h"

namespace pages
{
    // Example of a page whose content updates on its own over time,
    // independent of encoder input.
    class Page1 : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        unsigned long enteredAtMs_ = 0;
        unsigned long lastRedrawMs_ = 0;
    };

    extern Page1 page1;
}
