#include "process_calibration_page.h"
#include "page_id.h"
#include "managers/page_manager.h"

namespace pages
{
    ProcessCalibrationPage processCalibrationPage;
    void ProcessCalibrationPage::onEnter(TFT_eSPI &tft)
    {
        _selectedIndex = 0;

        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("PROCESS CALIBRATION", tft.width() / 2, 60);
    }

    void ProcessCalibrationPage::onExit()
    {
    }

    void ProcessCalibrationPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::Menu);
        }
    }
    void ProcessCalibrationPage::drawButton(TFT_eSPI &tft, int index)
    {
    }
}