#include "credential_page.h"
#include <stdio.h>
#include <string.h>
#include "managers/page_manager.h"
#include "theme.h"

namespace pages
{
    namespace t = theme;

    namespace
    {
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

        // Field card internals (offsets from the top of the card)
        constexpr int kLabelY = 24;
        constexpr int kValueY = 62;
        constexpr int kPillTop = 96;
        constexpr int kPillW = 100;
        constexpr int kPillH = 34;

        // Same green as the other pages' "active"/"saved" color
        constexpr uint16_t kOk = t::rgb(110, 200, 100);

        // X position of the card in a given slot: 0 = selected (centered),
        // -1 = left neighbour, +1 = right neighbour.
        int cardX(TFT_eSPI &tft, int slot)
        {
            return tft.width() / 2 - kCardW / 2 + slot * (kCardW + kCardGap);
        }

        // True if something centered at cx with width w lies fully on screen
        bool fitsOnScreen(TFT_eSPI &tft, int cx, int w)
        {
            return cx - w / 2 >= 0 && cx + w / 2 <= tft.width();
        }

        // Copies src into out, shortened with ".." so it fits maxW pixels in
        // the current font. outSize must be at least 4.
        void fitText(TFT_eSPI &tft, const char *src, int maxW, char *out, size_t outSize)
        {
            const size_t srcLen = strlen(src);
            if (srcLen < outSize && tft.textWidth(src) <= maxW)
            {
                memcpy(out, src, srcLen + 1);
                return;
            }

            size_t n = (srcLen < outSize - 3) ? srcLen : outSize - 3;
            for (; n > 0; n--)
            {
                memcpy(out, src, n);
                out[n] = '.';
                out[n + 1] = '.';
                out[n + 2] = '\0';
                if (tft.textWidth(out) <= maxW)
                {
                    return;
                }
            }
            out[0] = '\0';
        }
    }

    CredentialPage::CredentialPage(const char *title, const CredentialField *fields,
                                   int fieldCount, PageId backTarget)
        : _title(title), _fields(fields), _fieldCount(fieldCount), _backTarget(backTarget)
    {
    }

    int CredentialPage::wrapIndex(int i) const
    {
        const int n = itemCount();
        return ((i % n) + n) % n;
    }

    void CredentialPage::onEnter(TFT_eSPI &tft)
    {
        _selectedIndex = 0;
        _editingIndex = -1;
        _message = nullptr;
        _keyboard.close();

        drawScreen(tft);
    }

    void CredentialPage::onExit()
    {
        // Leaving while the keyboard is open (e.g. forced navigation) just
        // abandons the edit.
        _keyboard.close();
        _editingIndex = -1;
    }

    void CredentialPage::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        // ---- Keyboard open: it gets all the input ----
        if (_keyboard.isOpen())
        {
            const OnScreenKeyboard::Result result = _keyboard.update(tft, encoderDelta, buttonPressed);
            if (result != OnScreenKeyboard::Result::None)
            {
                finishEditing(tft, result);
            }
            return;
        }

        // ---- Browsing the fields ----

        // Any input clears the old "Saved" / error message
        if (_message != nullptr && (encoderDelta != 0 || buttonPressed))
        {
            _message = nullptr;
            drawHint(tft);
        }

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
            if (_selectedIndex == _fieldCount)
            {
                page_manager::navigateTo(_backTarget);
                return;
            }

            const CredentialField &field = _fields[_selectedIndex];
            const char *current = field.get ? field.get() : "";

            _editingIndex = _selectedIndex;
            _keyboard.open(tft, field.label, current, field.maxLength, field.numericOnly);
        }
    }

    void CredentialPage::finishEditing(TFT_eSPI &tft, OnScreenKeyboard::Result result)
    {
        _message = nullptr;

        if (result == OnScreenKeyboard::Result::Done && _editingIndex >= 0)
        {
            const CredentialField &field = _fields[_editingIndex];
            const char *newValue = _keyboard.text();
            const char *error = field.validate ? field.validate(newValue) : nullptr;

            if (error != nullptr)
            {
                _message = error;
                _messageIsError = true;
            }
            else
            {
                // Done editing: hand the new value to the network manager
                field.set(newValue);
                _message = "Saved";
                _messageIsError = false;
            }
        }

        _editingIndex = -1;

        // The keyboard used the whole screen, so redraw the page
        drawScreen(tft);
    }

    void CredentialPage::drawScreen(TFT_eSPI &tft)
    {
        const int cx = tft.width() / 2;

        tft.fillScreen(t::kBg);

        // Title + divider (same style as the other pages)
        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBg);
        tft.drawString(_title, cx, kTitleY);

        tft.drawFastHLine(cx - 70, kDividerY, 60, t::kSage);
        tft.drawFastHLine(cx + 10, kDividerY, 60, t::kSage);
        tft.fillCircle(cx, kDividerY, 3, t::kOchre);

        drawCarousel(tft);
        drawDots(tft);
        drawHint(tft);
    }

    void CredentialPage::drawHint(TFT_eSPI &tft)
    {
        tft.fillRect(0, kHintY - 10, tft.width(), 20, t::kBg);

        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(2);
        tft.setTextSize(1);

        if (_message != nullptr)
        {
            tft.setTextColor(_messageIsError ? t::kTerracotta : kOk, t::kBg);
            tft.drawString(_message, tft.width() / 2, kHintY);
        }
        else
        {
            tft.setTextColor(t::kSage, t::kBg);
            tft.drawString("Turn to scroll   |   Press to select", tft.width() / 2, kHintY);
        }
    }

    void CredentialPage::drawCarousel(TFT_eSPI &tft)
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

            drawCard(tft, wrapIndex(_selectedIndex + slot), x, slot == 0);

            if (slot < 1)
            {
                tft.fillRect(x + kCardW, kStripY, kCardGap, kStripH, t::kBg);
            }
        }
    }

    void CredentialPage::drawDots(TFT_eSPI &tft)
    {
        const int count = itemCount();
        const int totalW = (count - 1) * kDotSpacing;
        const int startX = tft.width() / 2 - totalW / 2;

        for (int i = 0; i < count; i++)
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

    void CredentialPage::drawCard(TFT_eSPI &tft, int itemIndex, int x, bool selected)
    {
        const bool isBack = (itemIndex == _fieldCount);
        const uint16_t fill = selected ? (isBack ? t::kBack : t::kLeafDark) : t::kCardIdle;
        const uint16_t border = selected ? t::kOchre : t::kLeafDark;

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

        // Text is only drawn if it fits fully on screen, so neighbour cards
        // cut off by the screen edge show just their shape (no half-clipped
        // text).
        if (isBack)
        {
            const char *label = "Back";
            tft.setTextFont(4);
            if (fitsOnScreen(tft, cx, tft.textWidth(label)))
            {
                tft.setTextColor(selected ? t::kCream : t::kSage, fill);
                tft.drawString(label, cx, kStripY + kStripH / 2);
            }
            return;
        }

        const CredentialField &field = _fields[itemIndex];

        // Small label on top
        tft.setTextFont(2);
        if (fitsOnScreen(tft, cx, tft.textWidth(field.label)))
        {
            tft.setTextColor(t::kSage, fill);
            tft.drawString(field.label, cx, kStripY + kLabelY);
        }

        // Current value in the middle (masked for passwords)
        const char *raw = field.get ? field.get() : "";
        if (raw == nullptr)
        {
            raw = "";
        }
        const bool isEmpty = (raw[0] == '\0');

        char display[72];
        if (isEmpty)
        {
            snprintf(display, sizeof(display), "(not set)");
        }
        else if (field.masked)
        {
            size_t n = strlen(raw);
            if (n > sizeof(display) - 1)
            {
                n = sizeof(display) - 1;
            }
            memset(display, '*', n);
            display[n] = '\0';
        }
        else
        {
            snprintf(display, sizeof(display), "%s", raw);
        }

        tft.setTextFont(4);
        char fitted[72];
        fitText(tft, display, kCardW - 32, fitted, sizeof(fitted));
        if (fitsOnScreen(tft, cx, tft.textWidth(fitted)))
        {
            tft.setTextColor(isEmpty ? t::kSage : (selected ? t::kCream : t::kSage), fill);
            tft.drawString(fitted, cx, kStripY + kValueY);
        }

        // "Edit" button at the bottom of the card
        if (fitsOnScreen(tft, cx, kPillW))
        {
            const uint16_t pillFill = selected ? t::kOchre : t::kLeafDark;
            tft.fillRoundRect(cx - kPillW / 2, kStripY + kPillTop, kPillW, kPillH, 10, pillFill);
            tft.setTextFont(4);
            tft.setTextColor(selected ? t::kBg : t::kSage, pillFill);
            tft.drawString("Edit", cx, kStripY + kPillTop + kPillH / 2);
        }
    }
}