#include "process_calibration_page.h"
#include "page_id.h"
#include "managers/page_manager.h"

namespace pages
{
    ProcessCalibrationPage processCalibrationPage;

    namespace
    {
        struct ProcessItem
        {
            const char *label;
            PageId target;
        };

        const ProcessItem items[] = {
            {"Hysteresis", PageId::HysteresisCalibration},
            {"Back", PageId::Setting},
        };

        const int itemCount = sizeof(items) / sizeof(items[0]);

        const int buttonHeight = 60;

        const int buttonGap = 15;
        const int marginX = 20;
        const int marginTop = 30;
    }

    void ProcessCalibrationPage::onEnter(TFT_eSPI &tft)
    {
        _selectedIndex = 0;

        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("PROCESS CALIBRATION", tft.width() / 2, 60);

        for (size_t i = 0; i < itemCount; i++)
        {
            drawButton(tft, i);
        }
    }

    void ProcessCalibrationPage::onExit()
    {
    }

    void ProcessCalibrationPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (encoderDelta != 0)
        {
            int previous = _selectedIndex;

            _selectedIndex = (_selectedIndex + static_cast<int>(encoderDelta)) % itemCount;
            if (_selectedIndex < 0)
            {
                _selectedIndex += itemCount;
            }

            if (_selectedIndex != previous)
            {
                drawButton(tft, previous);
                drawButton(tft, _selectedIndex);
            }
        }
        if (buttonPressed)
        {
            page_manager::navigateTo(items[_selectedIndex].target);
        }
    }
    void ProcessCalibrationPage::drawButton(TFT_eSPI &tft, int index)
    {
        int y = marginTop + index * (buttonHeight + buttonGap);
        bool selected = (index == _selectedIndex);
        uint16_t fillColor = selected ? (items[_selectedIndex].target == PageId::Setting ? TFT_RED : TFT_BLUE) : TFT_DARKGREY;

        tft.fillRoundRect(marginX, y, tft.width() - 2 * marginX, buttonHeight, 8, fillColor);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, fillColor);
        tft.setTextFont(4);
        tft.drawString(items[index].label, tft.width() / 2, y + buttonHeight / 2);
    }
}