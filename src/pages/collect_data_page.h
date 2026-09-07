#pragma once

#include "i_page.h"

namespace pages
{
    class CollectDataPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        enum class Focus
        {
            TemperatureSlider,
            TimerSlider,
            StartButton,
            BackButton
        };

        enum class SliderType
        {
            Temperature,
            Timer
        };

        void drawSlider(TFT_eSPI &tft, SliderType type);
        void drawButton(TFT_eSPI &tft, Focus which);
        void drawControl(TFT_eSPI &tft, Focus which);
        void startCollection();

        // Generalized per-slider-type lookups - same idea as drawButton()
        // already using `which` to look up a button's own y/label/color.
        // Add a third slider later by extending these five functions plus
        // one new Focus/SliderType value; drawSlider()/update() don't change.
        int &valueFor(SliderType type);
        int minFor(SliderType type) const;
        int maxFor(SliderType type) const;
        int anchorYFor(SliderType type) const;
        void labelFor(SliderType type, char *buffer, size_t bufferSize) const;
        static Focus focusFor(SliderType type);

        Focus _focus = Focus::TemperatureSlider;

        // Only one control can be "in edit mode" at a time - _focus already
        // says which one, so a single flag is enough (no per-slider flag
        // needed, and no way for two sliders to end up "editing" at once).
        bool _is_editing = false;

        int _current_temparture_celcius_value = 0;
        int _current_timer_minute_value = 0;
    };

    extern CollectDataPage collect_data_page;
}