#include <Arduino.h>
#include "steaming_summary_page.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include "managers/network_manager.h"
#include "managers/sensor_manager.h"
#include "theme.h"

namespace pages
{
    SteamingSummaryPage steaming_summary_page;
    namespace t = theme;

    namespace
    {
        // Layout (designed for 480x320) - same theme/card style as the other pages
        constexpr int kTitleY = 34;
        constexpr int kDividerY = 58;
        constexpr int kCardRadius = 12;

        constexpr int kCardTop = 84;
        constexpr int kCardH = 120;
        constexpr int kCardW = 208;
        constexpr int kCardGap = 16;

        constexpr int kBtnY = 222;
        constexpr int kBtnH = 48;
        constexpr int kBtnW = 240;

        constexpr int kHintY = 296;
    }

    void SteamingSummaryPage::onEnter(TFT_eSPI &tft)
    {
        const float setpoint = sensor_manager::get_setpoint_temperature();
        const int timerMinutes = static_cast<int>(sensor_manager::get_timer());
        Serial.printf("(Setpoint, Timer) = (%.2f, %d)\n", setpoint, timerMinutes);

        const int cx = tft.width() / 2;

        tft.fillScreen(t::kBg);

        // Title + divider (same style as the other pages)
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBg);
        tft.drawString("Steaming Summary", cx, kTitleY);

        tft.drawFastHLine(cx - 70, kDividerY, 60, t::kSage);
        tft.drawFastHLine(cx + 10, kDividerY, 60, t::kSage);
        tft.fillCircle(cx, kDividerY, 3, t::kOchre);

        // Setpoint is stored as a float; show it as a whole number when it is one
        char setpointText[16];
        if (setpoint == static_cast<int>(setpoint))
        {
            snprintf(setpointText, sizeof(setpointText), "%d", static_cast<int>(setpoint));
        }
        else
        {
            snprintf(setpointText, sizeof(setpointText), "%.1f", setpoint);
        }

        char timerText[16];
        snprintf(timerText, sizeof(timerText), "%d", timerMinutes);

        drawStatCard(tft, 0, "Set Temperature (C)", setpointText);
        drawStatCard(tft, 1, "Timer (min)", timerText);

        drawBackButton(tft);

        // Static hint
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(t::kSage, t::kBg);
        tft.drawString("Press button to return to menu", cx, kHintY);
    }

    void SteamingSummaryPage::onExit()
    {
    }

    void SteamingSummaryPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (buttonPressed)
        {
            // The sequence numbers are no longer shown on this page, but the
            // counters still have to start from zero for the next run.
            network_manager::reset_message_seq_id();
            page_manager::navigateTo(PageId::Menu);
        }
    }

    void SteamingSummaryPage::drawStatCard(TFT_eSPI &tft, int slot, const char *label, const char *value)
    {
        const int totalW = 2 * kCardW + kCardGap;
        const int x = (tft.width() - totalW) / 2 + slot * (kCardW + kCardGap);
        const int cx = x + kCardW / 2;

        tft.fillRoundRect(x, kCardTop, kCardW, kCardH, kCardRadius, t::kCardIdle);
        tft.drawRoundRect(x, kCardTop, kCardW, kCardH, kCardRadius, t::kLeafDark);

        tft.setTextDatum(MC_DATUM);

        // Small label on top (unit is part of the label so the value stays short)
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(t::kSage, t::kCardIdle);
        tft.drawString(label, cx, kCardTop + 26);

        // Big value in the middle
        tft.setTextFont(4);
        tft.setTextSize(2);
        tft.setTextColor(t::kCream, t::kCardIdle);
        tft.drawString(value, cx, kCardTop + 74);
        tft.setTextSize(1);
    }

    void SteamingSummaryPage::drawBackButton(TFT_eSPI &tft)
    {
        // The page has a single action, so the button is always "focused":
        // same look as a selected Back card on the carousel pages.
        const int x = (tft.width() - kBtnW) / 2;

        tft.fillRoundRect(x, kBtnY, kBtnW, kBtnH, kCardRadius, t::kBack);
        tft.drawRoundRect(x, kBtnY, kBtnW, kBtnH, kCardRadius, t::kOchre);
        tft.drawRoundRect(x + 1, kBtnY + 1, kBtnW - 2, kBtnH - 2, kCardRadius - 1, t::kOchre);

        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBack);
        tft.drawString("Back to Menu", tft.width() / 2, kBtnY + kBtnH / 2);
    }
}