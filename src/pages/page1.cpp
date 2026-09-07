#include "page1.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include <Arduino.h>

namespace pages
{
    Page1 page1;

    namespace
    {
        const int valueBoxY = 140;
        const int valueBoxHeight = 60;
    }

    void Page1::onEnter(TFT_eSPI &tft)
    {
        enteredAtMs_ = millis();
        lastRedrawMs_ = 0;

        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("Page 1 - Uptime", tft.width() / 2, 60);
        tft.setTextFont(2);
        tft.drawString("Press button to return to menu", tft.width() / 2, tft.height() - 30);
    }

    void Page1::onExit()
    {
    }

    void Page1::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        // Gate work with millis() instead of delay() so the button/encoder
        // stay responsive - update() must return quickly every call.
        unsigned long now = millis();
        if (now - lastRedrawMs_ >= 1000)
        {
            lastRedrawMs_ = now;
            unsigned long secondsUp = (now - enteredAtMs_) / 1000;

            tft.fillRect(0, valueBoxY, tft.width(), valueBoxHeight, TFT_BLACK);
            tft.setTextDatum(MC_DATUM);
            tft.setTextFont(6);
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.drawNumber(secondsUp, tft.width() / 2, valueBoxY + valueBoxHeight / 2);
        }

        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::Menu);
        }
    }
}
