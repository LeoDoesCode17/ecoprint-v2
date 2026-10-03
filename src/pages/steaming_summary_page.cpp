#include "steaming_summary_page.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include "managers/network_manager.h"

namespace pages
{
    SteamingSummaryPage steaming_summary_page;

    namespace
    {
        static char sensor_message_seq_id_label[64];
        static char actuator_message_seq_id_label[64];
    }

    void SteamingSummaryPage::onEnter(TFT_eSPI &tft)
    {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        tft.drawString("STEAMING SUMMARY", tft.width() / 2, tft.height() / 2 - 20);
        tft.setTextFont(2);
        snprintf(sensor_message_seq_id_label, sizeof(sensor_message_seq_id_label), "Sensor message seq numbers: %lu", network_manager::get_message_seq_id(0));
        snprintf(actuator_message_seq_id_label, sizeof(actuator_message_seq_id_label), "Actuator message seq numbers: %lu", network_manager::get_message_seq_id(1));
        tft.drawString(sensor_message_seq_id_label, tft.width() / 2, tft.height() / 2 + 10);
        tft.drawString(actuator_message_seq_id_label, tft.width() / 2, tft.height() / 2 + 40);
        tft.drawString("Press button to return to menu", tft.width() / 2, tft.height() - 30);
    }

    void SteamingSummaryPage::onExit()
    {
    }

    void SteamingSummaryPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (buttonPressed)
        {
            network_manager::reset_message_seq_id();
            page_manager::navigateTo(PageId::Menu);
        }
    }
}