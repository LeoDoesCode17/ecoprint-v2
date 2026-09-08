#include "page2.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include <Arduino.h>

namespace pages
{
    Page2 page2;

    namespace
    {
        const int valueBoxY = 140;
        const int valueBoxHeight = 60;
    }

    void Page2::onEnter(TFT_eSPI &tft)
    {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("Page 2 - Settings", tft.width() / 2, 60);
        tft.setTextFont(2);
        tft.drawString("Rotate to adjust, press to save & return", tft.width() / 2, tft.height() - 30);
        drawValue(tft);
    }

    void Page2::onExit()
    {
    }

    void Page2::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (encoderDelta != 0)
        {
            value_ += static_cast<int>(encoderDelta);
            value_ = constrain(value_, 0, 100);
            drawValue(tft);
        }

        if (buttonPressed)
        {
            // value_ is where you'd persist the setting, e.g. via
            // Preferences::putInt() before navigating away.
            page_manager::navigateTo(PageId::Menu);
        }
    }

    void Page2::drawValue(TFT_eSPI &tft)
    {
        tft.fillRect(0, valueBoxY, tft.width(), valueBoxHeight, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(6);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawNumber(value_, tft.width() / 2, valueBoxY + valueBoxHeight / 2);
    }
}
