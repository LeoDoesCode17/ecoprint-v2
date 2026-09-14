#pragma once
#include "i_page.h"

namespace pages
{
    class ThermocoupleCalibrationPage : public IPage
    {
    public:
        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        enum class Focus
        {
            IntegerSlider,
            TenthsSlider,
            HundredthsSlider,
            SignButton,
            ApplyButton,
            BackButton
        };

        enum class SliderType
        {
            IntegerPart,
            Tenths,
            Hundredths
        };

        void drawControl(TFT_eSPI &tft, Focus which);
        void drawSlider(TFT_eSPI &tft, SliderType type);
        void drawButton(TFT_eSPI &tft, Focus which);
        void applyOffset(TFT_eSPI &tft);
        void refreshLiveValueIfDue(TFT_eSPI &tft);
        void loadOffset(float offset);
        float currentOffset() const;

        // Generalized per-slider-type lookups - same pattern as Collect
        // Data's temperature/timer sliders, extended to a third slider
        // exactly the way that page's comments said it would look.
        int &valueFor(SliderType type);
        int minFor(SliderType type) const;
        int maxFor(SliderType type) const;
        int anchorYFor(SliderType type) const;
        void labelFor(SliderType type, char *buffer, size_t bufferSize) const;
        static Focus focusFor(SliderType type);
        static SliderType sliderTypeFor(Focus which);

        // Same idea again, one level down, for the row of three buttons.
        const char *buttonLabel(Focus which) const;
        static int buttonIndex(Focus which);
        void buttonGeometry(TFT_eSPI &tft, Focus which, int &x, int &w) const;

        Focus _focus = Focus::IntegerSlider;

        // Only one control can be "in edit mode" at a time - _focus already
        // says which slider, so a single flag is enough.
        bool _is_editing = false;

        bool _is_positive = true;
        int _integer_part = 0;
        int _tenths = 0;
        int _hundredths = 0;

        unsigned long _apply_flash_until_ms = 0;

        unsigned long _last_live_refresh_ms = 0;
        char _last_live_text[24];
    };

    extern ThermocoupleCalibrationPage thermocoupleCalibrationPage;
}