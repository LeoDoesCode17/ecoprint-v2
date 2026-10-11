#include <Arduino.h>
#include "menu_page.h"
#include "page_id.h"
#include "theme.h"
#include "managers/page_manager.h"

namespace pages
{
MenuPage menuPage;
namespace t = theme;

namespace
{
struct MenuItem
{
    const char *label;
    PageId target;
};

// Add/remove entries here to change the number of menu cards.
const MenuItem items[] = {
    // {"Live Reading", PageId::Page1},
    {"Test Actuator", PageId::TestActuator},
    {"Modify Servo", PageId::ModifyServo},
    {"Collect Data", PageId::CollectData},
    {"Setting", PageId::Setting},
};
const int itemCount = sizeof(items) / sizeof(items[0]);

// Layout (designed for 480x320)
constexpr int kTitleY = 34;
constexpr int kDividerY = 58;
constexpr int kStripY = 90;   // top of the card row
constexpr int kStripH = 140;  // card height
constexpr int kCardW = 240;
constexpr int kCardGap = 16;
constexpr int kCardRadius = 12;
constexpr int kDotsY = 262;
constexpr int kDotSpacing = 18;
constexpr int kHintY = 296;

// X position of the card in a given slot: 0 = selected (centered),
// -1 = left neighbour, +1 = right neighbour.
int cardX(TFT_eSPI &tft, int slot)
{
    return tft.width() / 2 - kCardW / 2 + slot * (kCardW + kCardGap);
}

int wrapIndex(int i)
{
    return ((i % itemCount) + itemCount) % itemCount;
}
} // namespace

void MenuPage::onEnter(TFT_eSPI &tft)
{
    selectedIndex_ = 0;
    const int cx = tft.width() / 2;

    tft.fillScreen(t::kBg);

    // Title + divider (same style as the landing page)
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(4);
    tft.setTextSize(1);
    tft.setTextColor(t::kCream, t::kBg);
    tft.drawString("Main Menu", cx, kTitleY);

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

void MenuPage::onExit()
{
    // nothing to clean up
}

void MenuPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
{
    if (encoderDelta != 0)
    {
        int previous = selectedIndex_;

        // Wrap around in both directions, however many steps the
        // encoder moved in one loop iteration.
        selectedIndex_ = wrapIndex(selectedIndex_ + static_cast<int>(encoderDelta));

        if (selectedIndex_ != previous)
        {
            drawCarousel(tft);
            drawDots(tft);
        }
    }

    if (buttonPressed)
    {
        page_manager::navigateTo(items[selectedIndex_].target);
    }
}

void MenuPage::drawCarousel(TFT_eSPI &tft)
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
        if (slot == 0 || itemCount >= 2)
        {
            drawCard(tft, wrapIndex(selectedIndex_ + slot), x, slot == 0);
        }
        if (slot < 1)
        {
            tft.fillRect(x + kCardW, kStripY, kCardGap, kStripH, t::kBg);
        }
    }
}

void MenuPage::drawCard(TFT_eSPI &tft, int itemIndex, int x, bool selected)
{
    const uint16_t fill = selected ? t::kLeafDark : t::kCardIdle;
    const uint16_t border = selected ? t::kOchre : t::kLeafDark;

    tft.fillRoundRect(x, kStripY, kCardW, kStripH, kCardRadius, fill);
    tft.drawRoundRect(x, kStripY, kCardW, kStripH, kCardRadius, border);
    if (selected)
    {
        // Thicker border for the selected card
        tft.drawRoundRect(x + 1, kStripY + 1, kCardW - 2, kStripH - 2,
                          kCardRadius - 1, border);
    }

    tft.setTextFont(4);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);

    // Only draw the label if it fits fully on screen. Neighbour cards that
    // are cut off by the screen edge show just their shape, which avoids
    // half-clipped text.
    const char *label = items[itemIndex].label;
    const int cx = x + kCardW / 2;
    const int tw = tft.textWidth(label);
    if (cx - tw / 2 >= 0 && cx + tw / 2 <= tft.width())
    {
        tft.setTextColor(selected ? t::kCream : t::kSage, fill);
        tft.drawString(label, cx, kStripY + kStripH / 2);
    }
}

void MenuPage::drawDots(TFT_eSPI &tft)
{
    const int totalW = (itemCount - 1) * kDotSpacing;
    const int startX = tft.width() / 2 - totalW / 2;

    for (int i = 0; i < itemCount; i++)
    {
        const int x = startX + i * kDotSpacing;
        tft.fillCircle(x, kDotsY, 6, t::kBg); // erase previous state
        if (i == selectedIndex_)
            tft.fillCircle(x, kDotsY, 5, t::kOchre);
        else
            tft.fillCircle(x, kDotsY, 3, t::kLeafMid);
    }
}
}