#pragma once

#include "i_page.h"

namespace pages
{
    // Shown after a run finishes or is stopped. Displays the setpoint
    // temperature and timer of the run; a click returns to the menu.
    class SteamingSummaryPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        // slot 0 = left card, slot 1 = right card
        void drawStatCard(TFT_eSPI &tft, int slot, const char *label, const char *value);
        void drawBackButton(TFT_eSPI &tft);
    };

    extern SteamingSummaryPage steaming_summary_page;
}