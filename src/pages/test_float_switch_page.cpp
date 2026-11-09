#include "test_float_switch_page.h"
#include <Arduino.h>
#include <stdio.h>
#include "page_id.h"
#include "managers/page_manager.h"
#include "managers/sensor_manager.h"
#include "sensors/float_switch.h" // adjust the path if your float_switch.h lives elsewhere
#include "config/pin.h"
#include "theme.h"

namespace pages
{
    TestFloatSwitchPage test_float_switch_page;
    namespace t = theme;

    namespace
    {
        // Layout (designed for 480x320) - same header as the other pages
        constexpr int kTitleY = 34;
        constexpr int kDividerY = 58;
        constexpr int kCardRadius = 12;

        // The two switch cards, side by side
        constexpr int kCardTop = 74;
        constexpr int kCardH = 112;
        constexpr int kCardW = 208;
        constexpr int kCardGap = 16;

        // Water status bar under the cards
        constexpr int kStatusX = 24;
        constexpr int kStatusY = 198;
        constexpr int kStatusW = 432; // = 2 * kCardW + kCardGap
        constexpr int kStatusH = 34;

        // Back button (the page's only action, so it is always highlighted)
        constexpr int kBtnY = 246;
        constexpr int kBtnH = 44;
        constexpr int kBtnW = 240;

        constexpr int kHintY = 306;

        // A bench-test page: refresh often so a flipped switch shows up fast
        constexpr unsigned long kRefreshMs = 200;

        // Same green as the other pages' "active" color
        constexpr uint16_t kOk = t::rgb(110, 200, 100);

        enum WaterLevel
        {
            kLack = 0,
            kSufficient = 1,
            kFull = 2
        };

        // Last values drawn (-1 = nothing drawn yet), so only what changed
        // is redrawn.
        int last_max_state = -1;
        int last_min_state = -1;
        int last_water_level = -1;
        unsigned long last_refresh_ms = 0;

        int readWaterLevel()
        {
            // Same rule as before: FULL wins over SUFFICIENT, otherwise LACK
            if (sensor_manager::is_water_full())
            {
                return kFull;
            }
            return sensor_manager::is_water_sufficient() ? kSufficient : kLack;
        }

        // slot 0 = left card (max), slot 1 = right card (min)
        void drawSwitchCard(TFT_eSPI &tft, int slot, const char *label, int gpio, int state)
        {
            const int totalW = 2 * kCardW + kCardGap;
            const int x = (tft.width() - totalW) / 2 + slot * (kCardW + kCardGap);
            const int cx = x + kCardW / 2;
            const bool isHigh = (state == HIGH);

            tft.fillRoundRect(x, kCardTop, kCardW, kCardH, kCardRadius, t::kCardIdle);
            tft.drawRoundRect(x, kCardTop, kCardW, kCardH, kCardRadius, t::kLeafDark);

            tft.setTextDatum(MC_DATUM);
            tft.setTextSize(1);

            // Small label on top
            tft.setTextFont(2);
            tft.setTextColor(t::kSage, t::kCardIdle);
            tft.drawString(label, cx, kCardTop + 20);

            // Big HIGH / LOW. This is the raw pin level: what it means
            // (water present or not) depends on how the switch is wired.
            tft.setTextFont(4);
            tft.setTextSize(2);
            tft.setTextColor(isHigh ? kOk : t::kSage, t::kCardIdle);
            tft.drawString(isHigh ? "HIGH" : "LOW", cx, kCardTop + 58);
            tft.setTextSize(1);

            // Which pin is being read
            char pinText[16];
            snprintf(pinText, sizeof(pinText), "GPIO %d", gpio);
            tft.setTextFont(2);
            tft.setTextColor(t::kSage, t::kCardIdle);
            tft.drawString(pinText, cx, kCardTop + 96);
        }

        void drawWaterStatus(TFT_eSPI &tft, int level)
        {
            const char *label = "Water status: ";
            const char *value = (level == kFull) ? "FULL" : (level == kSufficient ? "SUFFICIENT" : "LACK");
            const uint16_t valueColor = (level == kFull) ? kOk : (level == kSufficient ? t::kCream : t::kTerracotta);

            tft.fillRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 10, t::kCardIdle);
            tft.drawRoundRect(kStatusX, kStatusY, kStatusW, kStatusH, 10, t::kLeafDark);

            tft.setTextDatum(ML_DATUM);
            tft.setTextFont(2);
            tft.setTextSize(1);

            // Two colors on one centered line: label in sage, value colored
            const int labelW = tft.textWidth(label);
            const int valueW = tft.textWidth(value);
            const int startX = kStatusX + (kStatusW - (labelW + valueW)) / 2;
            const int y = kStatusY + kStatusH / 2;

            tft.setTextColor(t::kSage, t::kCardIdle);
            tft.drawString(label, startX, y);
            tft.setTextColor(valueColor, t::kCardIdle);
            tft.drawString(value, startX + labelW, y);
        }

        void drawBackButton(TFT_eSPI &tft)
        {
            // Same look as a selected Back card on the carousel pages
            const int x = (tft.width() - kBtnW) / 2;

            tft.fillRoundRect(x, kBtnY, kBtnW, kBtnH, kCardRadius, t::kBack);
            tft.drawRoundRect(x, kBtnY, kBtnW, kBtnH, kCardRadius, t::kOchre);
            tft.drawRoundRect(x + 1, kBtnY + 1, kBtnW - 2, kBtnH - 2, kCardRadius - 1, t::kOchre);

            tft.setTextDatum(MC_DATUM);
            tft.setTextFont(4);
            tft.setTextSize(1);
            tft.setTextColor(t::kCream, t::kBack);
            tft.drawString("Back", tft.width() / 2, kBtnY + kBtnH / 2);
        }

        // Reads everything and redraws only the parts whose value changed
        void refresh(TFT_eSPI &tft)
        {
            const int maxState = float_switch::get_max_float_switch_state();
            if (maxState != last_max_state)
            {
                last_max_state = maxState;
                drawSwitchCard(tft, 0, "Max Float Switch", static_cast<int>(pin::MAX_FLOAT_SWITCH_PIN), maxState);
            }

            const int minState = float_switch::get_min_float_switch_state();
            if (minState != last_min_state)
            {
                last_min_state = minState;
                drawSwitchCard(tft, 1, "Min Float Switch", static_cast<int>(pin::MIN_FLOAT_SWITCH_PIN), minState);
            }

            const int waterLevel = readWaterLevel();
            if (waterLevel != last_water_level)
            {
                last_water_level = waterLevel;
                drawWaterStatus(tft, waterLevel);
            }
        }
    }

    void TestFloatSwitchPage::onEnter(TFT_eSPI &tft)
    {
        const int cx = tft.width() / 2;

        tft.fillScreen(t::kBg);

        // Title + divider (same style as the other pages)
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBg);
        tft.drawString("Test Float Switch", cx, kTitleY);

        tft.drawFastHLine(cx - 70, kDividerY, 60, t::kSage);
        tft.drawFastHLine(cx + 10, kDividerY, 60, t::kSage);
        tft.fillCircle(cx, kDividerY, 3, t::kOchre);

        // Draw everything once; refresh() only redraws what changes later
        last_max_state = -1;
        last_min_state = -1;
        last_water_level = -1;
        refresh(tft);
        last_refresh_ms = millis();

        drawBackButton(tft);

        // Static hint
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(t::kSage, t::kBg);
        tft.drawString("Press button to go back", cx, kHintY);
    }

    void TestFloatSwitchPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        // Nothing to scroll or edit: the encoder is unused, a click goes back.
        (void)encoderDelta;

        if (buttonPressed)
        {
            page_manager::navigateTo(PageId::TestSensorList);
            return;
        }

        const unsigned long now = millis();
        if (now - last_refresh_ms >= kRefreshMs)
        {
            last_refresh_ms = now;
            refresh(tft);
        }
    }

    void TestFloatSwitchPage::onExit() {}
}