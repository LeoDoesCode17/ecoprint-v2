#include "page3.h"
#include "page_id.h"
#include "managers/page_manager.h"

namespace pages
{
    Page3 page3;

    void Page3::onEnter(TFT_eSPI &tft)
    {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("Page 3 - About", tft.width() / 2, tft.height() / 2 - 20);
        tft.setTextFont(2);
        tft.drawString("Multi-page demo firmware", tft.width() / 2, tft.height() / 2 + 10);
        tft.drawString("Press button to return to menu", tft.width() / 2, tft.height() - 30);
    }

    void Page3::onExit()
    {
    }

    void Page3::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::Menu);
        }
    }
}
