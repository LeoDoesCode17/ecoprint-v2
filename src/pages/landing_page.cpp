#include "landing_page.h"
#include "page_id.h"
#include "managers/page_manager.h"

namespace pages
{
    LandingPage landing_page;
    void LandingPage::onEnter(TFT_eSPI &tft)
    {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("HILIRISET ECOPRINT", tft.width() / 2, tft.height() / 2 - 20);
        tft.setTextFont(2);
        tft.drawString("Welcome to hiliriset ecoprint system", tft.width() / 2, tft.height() / 2 + 10);
        tft.drawString("Press button to return to proceed", tft.width() / 2, tft.height() - 30);
    }
    void LandingPage::onExit() {}
    void LandingPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::Menu);
        }
    }
}