#include "test_thermocouple_page.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include "managers/sensor_manager.h"

namespace pages
{
    TestThermocouplePage test_thermocouple_page;

    namespace
    {
        char thermocouple_sensor_label[64];
        float thermocouple_temperature;
        const unsigned long UPDATE_DISPLAY_INTERVAL_MS = 1000;
        unsigned long last_update_display = millis();
    }

    void TestThermocouplePage::onEnter(TFT_eSPI &tft)
    {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("TEST THERMOCOUPLE SENSOR", tft.width() / 2, tft.height() / 2 - 20);
        tft.setTextFont(2);
        thermocouple_temperature = sensor_manager::thermocouple_temperature_celcius();
        snprintf(thermocouple_sensor_label, sizeof(thermocouple_sensor_label), "Thermocoupel temperature: %f", thermocouple_temperature);
        tft.drawString(thermocouple_sensor_label, tft.width() / 2, tft.height() / 2 + 10);
        tft.drawString("Press button to return back to previous page", tft.width() / 2, tft.height() - 30);
    }

    void TestThermocouplePage::onExit() {}

    void TestThermocouplePage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (millis() - UPDATE_DISPLAY_INTERVAL_MS)
        {
            last_update_display = millis();
            tft.setTextFont(2);
            thermocouple_temperature = sensor_manager::thermocouple_temperature_celcius();
            snprintf(thermocouple_sensor_label, sizeof(thermocouple_sensor_label), "Thermocoupel temperature: %f", thermocouple_temperature);
            tft.drawString(thermocouple_sensor_label, tft.width() / 2, tft.height() / 2 + 10);
            tft.drawString("Press button to return to menu", tft.width() / 2, tft.height() - 30);
        }
        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::TestSensorList);
        }
    }

};