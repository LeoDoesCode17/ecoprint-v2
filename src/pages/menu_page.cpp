#include "menu_page.h"
#include "page_id.h"
#include "managers/page_manager.h"

namespace pages
{
    MenuPage menuPage;

    namespace
    {
        struct MenuItem
        {
            const char *label;
            PageId target;
        };

        // Add/remove entries here to change the number of menu buttons.
        const MenuItem items[] = {
            // {"Live Reading", PageId::Page1},
            {"Test Actuator", PageId::TestActuator},
            {"Modify Servo", PageId::ModifyServo},
            {"Collect Data", PageId::CollectData},
            {"Setting", PageId::Setting},
        };
        const int itemCount = sizeof(items) / sizeof(items[0]);

        const int buttonHeight = 60;
        const int buttonGap = 15;
        const int marginX = 20;
        const int marginTop = 30;
    }

    void MenuPage::onEnter(TFT_eSPI &tft)
    {
        selectedIndex_ = 0;
        tft.fillScreen(TFT_BLACK);
        for (int i = 0; i < itemCount; i++)
        {
            drawButton(tft, i);
        }
    }

    void MenuPage::onExit()
    {
        // nothing to clean up
    }

    void MenuPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (encoderDelta != 0)
        {
            int previous = selectedIndex_;

            // Wrap around in both directions, however many steps the
            // encoder moved in one loop iteration.
            selectedIndex_ = (selectedIndex_ + static_cast<int>(encoderDelta)) % itemCount;
            if (selectedIndex_ < 0)
            {
                selectedIndex_ += itemCount;
            }

            if (selectedIndex_ != previous)
            {
                // Only redraw the two buttons that actually changed -
                // redrawing the whole screen every tick causes visible
                // flicker on SPI TFTs.
                drawButton(tft, previous);
                drawButton(tft, selectedIndex_);
            }
        }

        if (buttonPressed)
        {
            page_manager::navigateTo(items[selectedIndex_].target);
        }
    }

    void MenuPage::drawButton(TFT_eSPI &tft, int index)
    {
        int y = marginTop + index * (buttonHeight + buttonGap);
        bool selected = (index == selectedIndex_);
        uint16_t fillColor = selected ? TFT_BLUE : TFT_DARKGREY;

        tft.fillRoundRect(marginX, y, tft.width() - 2 * marginX, buttonHeight, 8, fillColor);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, fillColor);
        tft.setTextFont(4);
        tft.drawString(items[index].label, tft.width() / 2, y + buttonHeight / 2);
    }
}
