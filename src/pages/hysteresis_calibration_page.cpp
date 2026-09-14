#include "hysteresis_calibration_page.h"
#include "page_id.h"
#include "managers/page_manager.h"

namespace pages
{
    HysteresisCalibrationPage hysteresisCalibrationPage;
    void HysteresisCalibrationPage::onEnter(TFT_eSPI &tft)
    {
        _selectedIndex = 0;

        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("HYSTERESIS CALIBRATION", tft.width() / 2, 60);
    }

    void HysteresisCalibrationPage::onExit()
    {
    }

    void HysteresisCalibrationPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::ProcessCalibration);
        }
    }
    void HysteresisCalibrationPage::drawButton(TFT_eSPI &tft, int index)
    {
    }
}