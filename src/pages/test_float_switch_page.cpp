#include "test_float_switch_page.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include "managers/sensor_manager.h"

namespace pages
{
    TestFloatSwitchPage test_float_switch_page;

    namespace
    {
        char water_status_label[64];
        bool is_water_sufficient, is_water_full;
        const unsigned long UPDATE_DISPLAY_INTERVAL_MS = 1000;
        unsigned long last_update_display = millis();
    }

    void TestFloatSwitchPage::onEnter(TFT_eSPI &tft)
    {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("TEST FLOAT SWITCH", tft.width() / 2, tft.height() / 2 - 20);
        tft.setTextFont(2);
        is_water_sufficient = sensor_manager::is_water_sufficient();
        is_water_full = sensor_manager::is_water_full();
        snprintf(water_status_label, sizeof(water_status_label), "Water status: %s", is_water_full ? "FULL" : (is_water_sufficient ? "SUFFIICIENT" : "LACK"));
        tft.drawString(water_status_label, tft.width() / 2, tft.height() / 2 + 10);
        tft.drawString("Press button to return to the previous page", tft.width() / 2, tft.height() - 30);
    }

    void TestFloatSwitchPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {

        if (millis() - last_update_display >= UPDATE_DISPLAY_INTERVAL_MS)
        {
            last_update_display = millis();
            tft.setTextFont(2);
            is_water_sufficient = sensor_manager::is_water_sufficient();
            is_water_full = sensor_manager::is_water_full();
            snprintf(water_status_label, sizeof(water_status_label), "Water status: %s", is_water_full ? "FULL" : (is_water_sufficient ? "SUFFIICIENT" : "LACK"));
            tft.drawString(water_status_label, tft.width() / 2, tft.height() / 2 + 10);
            tft.drawString("Press button to return to the previous page", tft.width() / 2, tft.height() - 30);
        }

        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::TestSensorList);
        }
    }
    void TestFloatSwitchPage::onExit() {}
}