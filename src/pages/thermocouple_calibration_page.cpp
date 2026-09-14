#include "thermocouple_calibration_page.h"
#include "page_id.h"
#include "managers/page_manager.h"

namespace pages
{
    ThermocoupleCalibrationPage thermocoupleCalibrationPage;

    void ThermocoupleCalibrationPage::onEnter(TFT_eSPI &tft)
    {
        _selectedIndex = 0;

        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("THERMOCOUPLE CALIBRATION", tft.width() / 2, 60);
    }

    void ThermocoupleCalibrationPage::onExit()
    {
    }

    void ThermocoupleCalibrationPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::SensorCalibration);
        }
    }
    void ThermocoupleCalibrationPage::drawButton(TFT_eSPI &tft, int index)
    {
    }
} // namespace pages
