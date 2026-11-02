#include "collect_data_page.h"
#include "page_id.h"
#include <Arduino.h>
#include "managers/page_manager.h"
#include "managers/sensor_manager.h"
#include "managers/state_manager.h"
#include "managers/queue_manager.h"
#include "theme.h"

namespace pages
{
    CollectDataPage collect_data_page;
    namespace t = theme;

    namespace
    {
        // Must match CollectDataPage::Card (checked by static_assert in onEnter)
        constexpr int kCardCount = 4;
        const char *const kLabels[kCardCount] = {"Temperature (C)", "Timer (min)", "Start", "Back"};

        // Slider ranges
        constexpr int kTempMin = 50;
        constexpr int kTempMax = 100;
        constexpr int kTimerMin = 30;
        constexpr int kTimerMax = 120;

        // Layout (designed for 480x320) - same as the other carousel pages
        constexpr int kTitleY = 34;
        constexpr int kDividerY = 58;
        constexpr int kStripY = 90;  // top of the card row
        constexpr int kStripH = 140; // card height
        constexpr int kCardW = 240;
        constexpr int kCardGap = 16;
        constexpr int kCardRadius = 12;
        constexpr int kDotsY = 262;
        constexpr int kDotSpacing = 18;
        constexpr int kHintY = 296;

        // Slider card internals
        constexpr int kBarMarginX = 24; // bar inset from the card edges
        constexpr int kBarH = 14;

        // Border/bar color while a slider is in edit mode
        constexpr uint16_t kEditing = t::rgb(110, 200, 100);

        // X position of the card in a given slot: 0 = selected (centered),
        // -1 = left neighbour, +1 = right neighbour.
        int cardX(TFT_eSPI &tft, int slot)
        {
            return tft.width() / 2 - kCardW / 2 + slot * (kCardW + kCardGap);
        }

        int wrapIndex(int i)
        {
            return ((i % kCardCount) + kCardCount) % kCardCount;
        }

        // True if something centered at cx with width w lies fully on screen
        bool fitsOnScreen(TFT_eSPI &tft, int cx, int w)
        {
            return cx - w / 2 >= 0 && cx + w / 2 <= tft.width();
        }
    }

    void CollectDataPage::onEnter(TFT_eSPI &tft)
    {
        static_assert(static_cast<int>(Card::Count) == kCardCount,
                      "kCardCount / kLabels must match CollectDataPage::Card");

        _selectedIndex = 0;
        _is_editing = false;
        // Start from the lowest valid value so the card never shows an
        // out-of-range "0" and Start always sends valid numbers.
        _current_temparture_celcius_value = kTempMin;
        _current_timer_minute_value = kTimerMin;

        const int cx = tft.width() / 2;

        tft.fillScreen(t::kBg);

        // Title + divider (same style as the landing page)
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBg);
        tft.drawString("Collect Data", cx, kTitleY);

        tft.drawFastHLine(cx - 70, kDividerY, 60, t::kSage);
        tft.drawFastHLine(cx + 10, kDividerY, 60, t::kSage);
        tft.fillCircle(cx, kDividerY, 3, t::kOchre);

        drawHint(tft);
        drawCarousel(tft);
        drawDots(tft);
    }

    void CollectDataPage::onExit()
    {
        _is_editing = false;
    }

    void CollectDataPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        const Card current = static_cast<Card>(_selectedIndex);

        // ---- Edit mode: the encoder changes the value, a click confirms ----
        if (_is_editing)
        {
            if (encoderDelta != 0)
            {
                int &value = valueFor(current);
                const int previous = value;
                value = constrain(value + static_cast<int>(encoderDelta),
                                  minFor(current), maxFor(current));

                if (value != previous)
                {
                    // Only the selected card changed
                    drawCard(tft, current, cardX(tft, 0), true);
                }
            }

            if (buttonPressed)
            {
                _is_editing = false;
                drawCard(tft, current, cardX(tft, 0), true);
                drawHint(tft);
            }
            return;
        }

        // ---- Browse mode: the encoder scrolls the carousel ----
        if (encoderDelta != 0)
        {
            const int previous = _selectedIndex;

            // Wrap around in both directions, however many steps the
            // encoder moved in one loop iteration.
            _selectedIndex = wrapIndex(_selectedIndex + static_cast<int>(encoderDelta));

            if (_selectedIndex != previous)
            {
                drawCarousel(tft);
                drawDots(tft);
            }
        }

        if (buttonPressed)
        {
            switch (current)
            {
            case Card::Temperature:
            case Card::Timer:
                _is_editing = true;
                drawCard(tft, current, cardX(tft, 0), true);
                drawHint(tft);
                break;

            case Card::Start:
                startCollection();
                break;

            case Card::Back:
                page_manager::navigateTo(PageId::Menu);
                break;

            default:
                break;
            }
        }
    }

    void CollectDataPage::drawCarousel(TFT_eSPI &tft)
    {
        // Every pixel of the strip is overwritten exactly once (either by a
        // card or by a background-colored gap), so there is no full-clear
        // flicker when scrolling.
        const int left = cardX(tft, -1);
        const int right = cardX(tft, 1) + kCardW;
        if (left > 0)
            tft.fillRect(0, kStripY, left, kStripH, t::kBg);
        if (right < tft.width())
            tft.fillRect(right, kStripY, tft.width() - right, kStripH, t::kBg);

        for (int slot = -1; slot <= 1; slot++)
        {
            const int x = cardX(tft, slot);

            drawCard(tft, static_cast<Card>(wrapIndex(_selectedIndex + slot)), x, slot == 0);

            if (slot < 1)
            {
                tft.fillRect(x + kCardW, kStripY, kCardGap, kStripH, t::kBg);
            }
        }
    }

    void CollectDataPage::drawCard(TFT_eSPI &tft, Card card, int x, bool selected)
    {
        const int index = static_cast<int>(card);
        const bool isBack = (card == Card::Back);
        const bool editing = selected && _is_editing; // only the selected card can be edited

        const uint16_t fill = selected ? (isBack ? t::kBack : t::kLeafDark) : t::kCardIdle;
        const uint16_t border = editing ? kEditing : (selected ? t::kOchre : t::kLeafDark);

        tft.fillRoundRect(x, kStripY, kCardW, kStripH, kCardRadius, fill);
        tft.drawRoundRect(x, kStripY, kCardW, kStripH, kCardRadius, border);
        if (selected)
        {
            // Thicker border for the selected card
            tft.drawRoundRect(x + 1, kStripY + 1, kCardW - 2, kStripH - 2,
                              kCardRadius - 1, border);
        }

        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);

        const int cx = x + kCardW / 2;
        const int midY = kStripY + kStripH / 2;
        const uint16_t mainText = selected ? t::kCream : t::kSage;

        // Text is only drawn if it fits fully on screen, so neighbour cards
        // cut off by the screen edge show just their shape (no half-clipped
        // text).
        if (isSlider(card))
        {
            // Small label on top
            tft.setTextFont(2);
            if (fitsOnScreen(tft, cx, tft.textWidth(kLabels[index])))
            {
                tft.setTextColor(t::kSage, fill);
                tft.drawString(kLabels[index], cx, kStripY + 24);
            }

            // Big value in the middle
            char valueText[8];
            snprintf(valueText, sizeof(valueText), "%d", valueFor(card));
            tft.setTextFont(4);
            tft.setTextSize(2);
            if (fitsOnScreen(tft, cx, tft.textWidth(valueText)))
            {
                tft.setTextColor(mainText, fill);
                tft.drawString(valueText, cx, kStripY + 66);
            }
            tft.setTextSize(1);

            // Progress bar showing where the value sits in its range
            const int barW = kCardW - 2 * kBarMarginX;
            if (fitsOnScreen(tft, cx, barW + 4))
            {
                const int barX = x + kBarMarginX;
                const int barY = kStripY + 108;
                const int fillW = map(valueFor(card), minFor(card), maxFor(card), 0, barW);

                tft.fillRect(barX, barY, barW, kBarH, t::kBg);
                if (fillW > 0)
                {
                    tft.fillRect(barX, barY, fillW, kBarH, editing ? kEditing : t::kOchre);
                }
                tft.drawRect(barX - 2, barY - 2, barW + 4, kBarH + 4, t::kLeafMid);
            }
        }
        else
        {
            const bool isStart = (card == Card::Start);

            tft.setTextFont(4);
            if (fitsOnScreen(tft, cx, tft.textWidth(kLabels[index])))
            {
                tft.setTextColor(mainText, fill);
                tft.drawString(kLabels[index], cx, isStart ? midY - 14 : midY);
            }

            // Start card also summarises what is about to be started
            if (isStart)
            {
                char summary[32];
                snprintf(summary, sizeof(summary), "%d C  /  %d min",
                         _current_temparture_celcius_value, _current_timer_minute_value);
                tft.setTextFont(2);
                if (fitsOnScreen(tft, cx, tft.textWidth(summary)))
                {
                    tft.setTextColor(t::kSage, fill);
                    tft.drawString(summary, cx, midY + 22);
                }
            }
        }
    }

    void CollectDataPage::drawDots(TFT_eSPI &tft)
    {
        const int totalW = (kCardCount - 1) * kDotSpacing;
        const int startX = tft.width() / 2 - totalW / 2;

        for (int i = 0; i < kCardCount; i++)
        {
            const int x = startX + i * kDotSpacing;
            tft.fillCircle(x, kDotsY, 6, t::kBg); // erase previous state
            if (i == _selectedIndex)
            {
                tft.fillCircle(x, kDotsY, 5, t::kOchre);
            }
            else
            {
                tft.fillCircle(x, kDotsY, 3, t::kLeafMid);
            }
        }
    }

    void CollectDataPage::drawHint(TFT_eSPI &tft)
    {
        // The hint changes between browse mode and edit mode
        tft.fillRect(0, kHintY - 10, tft.width(), 20, t::kBg);
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(_is_editing ? kEditing : t::kSage, t::kBg);
        tft.drawString(_is_editing ? "Turn to adjust   |   Press to confirm"
                                   : "Turn to scroll   |   Press to select",
                       tft.width() / 2, kHintY);
    }

    void CollectDataPage::startCollection()
    {
        // Hand the configured values off to page_manager so the Sensor
        // Dashboard (or anything else) can read them after we navigate away -
        // this page's own members disappear the moment we leave it.
        sensor_manager::set_setpoint_temperature((float)_current_temparture_celcius_value);
        sensor_manager::set_timer(_current_timer_minute_value);
        state_manager::set_state_machine(StateMachine::IDLE);
        const command_t start_command = {.command = 1, .setpoint = _current_temparture_celcius_value, .timer_minutes = _current_timer_minute_value, .source = command_source_t::LOCAL};
        queue_manager::enqueue_command_data(start_command);
        page_manager::navigateTo(PageId::SensorDashboard);
    }

    bool CollectDataPage::isSlider(Card card)
    {
        return card == Card::Temperature || card == Card::Timer;
    }

    int &CollectDataPage::valueFor(Card card)
    {
        return (card == Card::Temperature) ? _current_temparture_celcius_value : _current_timer_minute_value;
    }

    int CollectDataPage::minFor(Card card)
    {
        return (card == Card::Temperature) ? kTempMin : kTimerMin;
    }

    int CollectDataPage::maxFor(Card card)
    {
        return (card == Card::Temperature) ? kTempMax : kTimerMax;
    }
}