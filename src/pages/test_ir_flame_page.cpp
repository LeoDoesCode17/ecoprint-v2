#include "test_ir_flame_page.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include "managers/sensor_manager.h"

namespace pages
{
    TestIrFlamePage test_ir_flame_page;

    namespace
    {
        char ir_flame_state_label[64];
        bool is_fire_detected;
        const unsigned long UPDATE_DISPLAY_INTERVAL_MS = 1000;
        unsigned long last_update_display = millis();
    }

    void TestIrFlamePage::onEnter(TFT_eSPI &tft)
    {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("TEST IR FLAME SENSOR", tft.width() / 2, tft.height() / 2 - 20);
        tft.setTextFont(2);
        is_fire_detected = sensor_manager::is_fire_detected();
        snprintf(ir_flame_state_label, sizeof(ir_flame_state_label), "FIRE STATUS: %s", is_fire_detected ? "DETECTED" : "UNDETECTED");
        tft.drawString(ir_flame_state_label, tft.width() / 2, tft.height() / 2 + 10);
        tft.drawString("Press button to return to the previous page", tft.width() / 2, tft.height() - 30);
    }

    void TestIrFlamePage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (millis() - last_update_display >= UPDATE_DISPLAY_INTERVAL_MS)
        {
            snprintf(ir_flame_state_label, sizeof(ir_flame_state_label), "");
            last_update_display = millis();
            tft.setTextFont(2);
            is_fire_detected = sensor_manager::is_fire_detected();
            snprintf(ir_flame_state_label, sizeof(ir_flame_state_label), "FIRE STATUS: %s", is_fire_detected ? "DETECTED" : "UNDETECTED");
            tft.drawString(ir_flame_state_label, tft.width() / 2, tft.height() / 2 + 10);
            tft.drawString("Press button to return to the previous page", tft.width() / 2, tft.height() - 30);
        }
        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::TestSensorList);
        }
    }
    
    void TestIrFlamePage::onExit() {}
}