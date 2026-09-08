#pragma once
#include "i_page.h"

namespace pages
{
    // Example of the simplest possible page: fully static, no per-frame work.
    class Page3 : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;
    };

    extern Page3 page3;
}
