#include "test_actuator_page.h"
#include "page_id.h"
#include "managers/page_manager.h"
#include <Arduino.h>
#include "managers/actuator_manager.h"
#include "theme.h"

namespace pages
{
    TestActuatorPage test_actuator_page;
    namespace t = theme;

    namespace
    {
        struct ActuatorItem
        {
            const char *label;
            const int actuator_id;
            bool is_active;
        };

        ActuatorItem items[] = {
            {"Test Pump", 0, false},
            {"Test Lighter", 1, false},
            {"Test Valve", 2, false},
            {"Back", 3, false}};

        const int item_count = sizeof(items) / sizeof(items[0]);

        const char *on_label = "ON";
        const char *off_label = "OFF";

        // Layout (designed for 480x320)
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

        // Status pill + Back card id
        constexpr int kBackId = 3;
        constexpr int kPillW = 90;
        constexpr int kPillH = 32;
        constexpr uint16_t kStatusOn = t::rgb(110, 200, 100); // bright leaf green

        bool fitsOnScreen(TFT_eSPI &tft, int cx, int w)
        {
            return cx - w / 2 >= 0 && cx + w / 2 <= tft.width();
        }

        // Drives the real actuator for a given id
        void applyActuator(int id, bool on)
        {
            switch (id)
            {
            case 0: // pump
                Serial.println(on ? "[ACTUATOR] Turn on pump" : "[ACTUATOR] Turn off pump");
                if (on)
                    actuator_manager::turn_on_pump();
                else
                    actuator_manager::turn_off_pump();
                break;
            case 1: // lighter
                Serial.println(on ? "[ACTUATOR] Turn on lighter" : "[ACTUATOR] Turn off lighter");
                if (on)
                    actuator_manager::turn_on_lighter();
                else
                    actuator_manager::turn_off_lighter();
                break;
            case 2: // servo valve
                Serial.println(on ? "[ACTUATOR] Open servo valve" : "[ACTUATOR] Close servo valve");
                if (on)
                    actuator_manager::open_valve_by_degree(90);
                else
                    actuator_manager::close_valve();
                break;
            }
        }

        // X position of the card in a given slot: 0 = selected (centered),
        // -1 = left neighbour, +1 = right neighbour.
        int cardX(TFT_eSPI &tft, int slot)
        {
            return tft.width() / 2 - kCardW / 2 + slot * (kCardW + kCardGap);
        }

        int wrapIndex(int i)
        {
            return ((i % item_count) + item_count) % item_count;
        }
    }

    void TestActuatorPage::onEnter(TFT_eSPI &tft)
    {
        _selectedIndex = 0;
        const int cx = tft.width() / 2;

        tft.fillScreen(t::kBg);

        // Title + divider (same style as the landing page)
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBg);
        tft.drawString("Test Actuator Menu", cx, kTitleY);

        tft.drawFastHLine(cx - 70, kDividerY, 60, t::kSage);
        tft.drawFastHLine(cx + 10, kDividerY, 60, t::kSage);
        tft.fillCircle(cx, kDividerY, 3, t::kOchre);

        // Static hint
        tft.setTextFont(2);
        tft.setTextColor(t::kSage, t::kBg);
        tft.drawString("Turn to scroll   |   Press to select", cx, kHintY);

        drawCarousel(tft);
        drawDots(tft);
    }

    void TestActuatorPage::onExit()
    {
        for (ActuatorItem &item : items)
        {
            if (item.actuator_id != kBackId && item.is_active)
            {
                item.is_active = false;
                applyActuator(item.actuator_id, false);
            }
        }
    }

    void TestActuatorPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (encoderDelta != 0)
        {
            int previous = _selectedIndex;
            _selectedIndex = wrapIndex(_selectedIndex + static_cast<int>(encoderDelta));
            if (_selectedIndex != previous)
            {
                drawCarousel(tft);
                drawDots(tft);
            }
        }

        if (buttonPressed)
        {
            ActuatorItem &item = items[_selectedIndex];

            if (item.actuator_id == kBackId)
            {
                Serial.println("[UI] Back to menu page");
                page_manager::navigateTo(PageId::Menu);
                return; // don't draw anything after leaving the page
            }

            item.is_active = !item.is_active;
            applyActuator(item.actuator_id, item.is_active);

            // Only the selected card changed, so redraw just that one
            drawCard(tft, _selectedIndex, cardX(tft, 0), true);
        }
    }

    void TestActuatorPage::drawCarousel(TFT_eSPI &tft)
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

            // With a single item there are no neighbours to show.
            if (slot == 0 || item_count >= 2)
            {
                drawCard(tft, wrapIndex(_selectedIndex + slot), x, slot == 0);
            }
            if (slot < 1)
            {
                tft.fillRect(x + kCardW, kStripY, kCardGap, kStripH, t::kBg);
            }
        }
    }

    void TestActuatorPage::drawDots(TFT_eSPI &tft)
    {
        const int totalW = (item_count - 1) * kDotSpacing;
        const int startX = tft.width() / 2 - totalW / 2;

        for (int i = 0; i < item_count; i++)
        {
            const int x = startX + i * kDotSpacing;
            tft.fillCircle(x, kDotsY, 6, t::kBg); // erase previous state
            if (i == _selectedIndex)
                tft.fillCircle(x, kDotsY, 5, t::kOchre);
            else
                tft.fillCircle(x, kDotsY, 3, t::kLeafMid);
        }
    }

    void TestActuatorPage::drawCard(TFT_eSPI &tft, int itemIndex, int x, bool selected)
    {
        const ActuatorItem &item = items[itemIndex];
        const bool isBack = (item.actuator_id == kBackId);

        const uint16_t fill = selected ? (isBack ? t::kBack : t::kLeafDark) : t::kCardIdle;
        const uint16_t border = selected ? t::kOchre : t::kLeafDark;

        tft.fillRoundRect(x, kStripY, kCardW, kStripH, kCardRadius, fill);
        tft.drawRoundRect(x, kStripY, kCardW, kStripH, kCardRadius, border);
        if (selected)
        {
            tft.drawRoundRect(x + 1, kStripY + 1, kCardW - 2, kStripH - 2,
                              kCardRadius - 1, border);
        }

        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);

        const int cx = x + kCardW / 2;
        const int midY = kStripY + kStripH / 2;

        // Label: centered on the Back card, shifted up to make room for the
        // status pill on actuator cards.
        const int tw = tft.textWidth(item.label);
        if (fitsOnScreen(tft, cx, tw))
        {
            tft.setTextColor(selected ? t::kCream : t::kSage, fill);
            tft.drawString(item.label, cx, isBack ? midY : midY - 22);
        }

        // Status pill (actuator cards only), skipped if cut off by the screen edge
        if (!isBack && fitsOnScreen(tft, cx, kPillW))
        {
            const bool on = item.is_active;
            const uint16_t pillFill = on ? kStatusOn : t::kTerracotta;
            const uint16_t pillText = on ? t::kBg : t::kCream;
            const int pillY = midY + 28;

            tft.fillRoundRect(cx - kPillW / 2, pillY - kPillH / 2, kPillW, kPillH,
                              kPillH / 2, pillFill);
            tft.setTextColor(pillText, pillFill);
            tft.drawString(on ? on_label : off_label, cx, pillY);
        }
    }

}