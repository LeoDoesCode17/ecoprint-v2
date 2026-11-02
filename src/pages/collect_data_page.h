#pragma once

#include "i_page.h"

namespace pages
{
    // Horizontal carousel with four cards: Temperature, Timer, Start, Back.
    //
    // Scrolling: turn the encoder to move between cards.
    // Temperature / Timer cards: press to enter edit mode (green border),
    //   turn to change the value, press again to confirm.
    // Start / Back cards: press to activate.
    class CollectDataPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        // Order must match kLabels[] in collect_data_page.cpp
        enum class Card
        {
            Temperature,
            Timer,
            Start,
            Back,
            Count // not a real card - used only to size loops
        };

        void drawCarousel(TFT_eSPI &tft);
        void drawCard(TFT_eSPI &tft, Card card, int x, bool selected);
        void drawDots(TFT_eSPI &tft);
        void drawHint(TFT_eSPI &tft);
        void startCollection();

        // Slider cards only (Temperature / Timer)
        static bool isSlider(Card card);
        int &valueFor(Card card);
        static int minFor(Card card);
        static int maxFor(Card card);

        int _selectedIndex = 0;

        // Only the selected card can be in edit mode, so one flag is enough
        // (there is no way for two sliders to be "editing" at once).
        bool _is_editing = false;

        int _current_temparture_celcius_value = 0;
        int _current_timer_minute_value = 0;
    };

    extern CollectDataPage collect_data_page;
}