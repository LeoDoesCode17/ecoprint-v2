#include "test_sht_page.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include "managers/sensor_manager.h"

namespace pages
{
    TestShtPage test_sht_page;

    namespace
    {
        char sht_temperature_label[64];
        char sht_humidity_label[64];
        float sht_temperature_celcius, sht_humidity_percent;
        const unsigned long UPDATE_DISPLAY_INTERVAL_MS = 1000;
        unsigned long last_update_display = millis();
    }

    void TestShtPage::onEnter(TFT_eSPI &tft)
    {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("TEST SHT3X SENSOR", tft.width() / 2, tft.height() / 2 - 20);
        tft.setTextFont(2);
        sht_temperature_celcius = sensor_manager::sht3x_temperature_celcius();
        sht_humidity_percent = sensor_manager::sht3x_humidity_percent();
        snprintf(sht_temperature_label, sizeof(sht_temperature_label), "SHT Temperature celcius: %.2f", sht_temperature_celcius);
        snprintf(sht_humidity_label, sizeof(sht_humidity_label), "SHT Humdity percent: %.2f%", sht_humidity_percent);
        tft.drawString(sht_temperature_label, tft.width() / 2, tft.height() / 2 + 10);
        tft.drawString(sht_humidity_label, tft.width() / 2, tft.height() / 2 + 40);
        tft.drawString("Press button to return to the previous page", tft.width() / 2, tft.height() - 30);
    }

    void TestShtPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (millis() - last_update_display >= UPDATE_DISPLAY_INTERVAL_MS)
        {
            last_update_display = millis();
            tft.setTextFont(2);
            sht_temperature_celcius = sensor_manager::sht3x_temperature_celcius();
            sht_humidity_percent = sensor_manager::sht3x_humidity_percent();
            snprintf(sht_temperature_label, sizeof(sht_temperature_label), "SHT Temperature celcius: %.2f", sht_temperature_celcius);
            snprintf(sht_humidity_label, sizeof(sht_humidity_label), "SHT Humdity percent: %.2f%", sht_humidity_percent);
            tft.drawString(sht_temperature_label, tft.width() / 2, tft.height() / 2 + 10);
            tft.drawString(sht_humidity_label, tft.width() / 2, tft.height() / 2 + 40);
            tft.drawString("Press button to return to the previous page", tft.width() / 2, tft.height() - 30);
        }
        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::TestSensorList);
        }
    }

    void TestShtPage::onExit() {}

}