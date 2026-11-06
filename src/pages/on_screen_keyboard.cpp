#include "on_screen_keyboard.h"
#include <stdio.h>
#include <string.h>
#include "theme.h"

namespace pages
{
    namespace t = theme;

    namespace
    {
        // Layout (designed for 480x320)
        constexpr int kRows = 4;
        constexpr int kCols = 10;
        constexpr int kKeyW = 39;
        constexpr int kKeyH = 34;
        constexpr int kKeyGap = 6;
        constexpr int kKeyRadius = 6;
        constexpr int kKeysTop = 96;
        constexpr int kFnY = 262;
        constexpr int kFnH = 38;

        constexpr int kTitleY = 20;

        // Text box that shows what has been typed so far
        constexpr int kBoxX = 16;
        constexpr int kBoxY = 46;
        constexpr int kBoxW = 448;
        constexpr int kBoxH = 38;

        // Character layers: 4 rows each, up to 10 keys per row
        const char *const kLower[kRows] = {"abcdefghij", "klmnopqrst", "uvwxyz0123", "456789-_.@"};
        const char *const kUpper[kRows] = {"ABCDEFGHIJ", "KLMNOPQRST", "UVWXYZ0123", "456789-_.@"};
        const char *const kSymbols[kRows] = {"!\"#$%&'()*", "+,-./:;<=>", "?@[\\]^_`{|", "}~"};
        const char *const kNumeric[kRows] = {"0123456789", "", "", ""};
    }

    // ------------------------------------------------------------------
    // Layout helpers
    // ------------------------------------------------------------------

    const char *OnScreenKeyboard::rowChars(int row) const
    {
        switch (_layer)
        {
        case Layer::Upper:
            return kUpper[row];
        case Layer::Symbols:
            return kSymbols[row];
        case Layer::Numeric:
            return kNumeric[row];
        case Layer::Lower:
        default:
            return kLower[row];
        }
    }

    int OnScreenKeyboard::charKeyCount() const
    {
        int count = 0;
        for (int row = 0; row < kRows; row++)
        {
            count += static_cast<int>(strlen(rowChars(row)));
        }
        return count;
    }

    char OnScreenKeyboard::charAt(int index) const
    {
        for (int row = 0; row < kRows; row++)
        {
            const int len = static_cast<int>(strlen(rowChars(row)));
            if (index < len)
            {
                return rowChars(row)[index];
            }
            index -= len;
        }
        return '\0';
    }

    int OnScreenKeyboard::fnKeyCount() const
    {
        return _numericOnly ? 3 : 5;
    }

    OnScreenKeyboard::FnKey OnScreenKeyboard::fnKeyAt(int fnIndex) const
    {
        static const FnKey kFull[] = {FnKey::Mode, FnKey::Space, FnKey::Delete, FnKey::Cancel, FnKey::Done};
        static const FnKey kNumericOnly[] = {FnKey::Delete, FnKey::Cancel, FnKey::Done};
        return _numericOnly ? kNumericOnly[fnIndex] : kFull[fnIndex];
    }

    // Relative width of each function key
    int OnScreenKeyboard::fnWeight(FnKey key)
    {
        return (key == FnKey::Space) ? 3 : 2;
    }

    // The Mode key shows the layer you will get when you press it
    const char *OnScreenKeyboard::fnLabel(FnKey key, Layer layer)
    {
        switch (key)
        {
        case FnKey::Mode:
            return (layer == Layer::Lower) ? "ABC" : (layer == Layer::Upper) ? "#@!"
                                                                             : "abc";
        case FnKey::Space:
            return "Space";
        case FnKey::Delete:
            return "Del";
        case FnKey::Cancel:
            return "Cancel";
        case FnKey::Done:
            return "Done";
        }
        return "";
    }

    const char *OnScreenKeyboard::layerName() const
    {
        switch (_layer)
        {
        case Layer::Upper:
            return "UPPERCASE";
        case Layer::Symbols:
            return "symbols";
        case Layer::Numeric:
            return "digits";
        case Layer::Lower:
        default:
            return "lowercase";
        }
    }

    void OnScreenKeyboard::keyRect(TFT_eSPI &tft, int index, int &x, int &y, int &w, int &h) const
    {
        const int charCount = charKeyCount();

        if (index < charCount)
        {
            // Find the row/column of this character key
            int row = 0;
            int col = index;
            while (row < kRows - 1)
            {
                const int len = static_cast<int>(strlen(rowChars(row)));
                if (col < len)
                {
                    break;
                }
                col -= len;
                row++;
            }

            // Rows are centered, so short rows (symbols) stay tidy
            const int rowLen = static_cast<int>(strlen(rowChars(row)));
            const int rowW = rowLen * kKeyW + (rowLen - 1) * kKeyGap;
            x = (tft.width() - rowW) / 2 + col * (kKeyW + kKeyGap);
            y = kKeysTop + row * (kKeyH + kKeyGap);
            w = kKeyW;
            h = kKeyH;
            return;
        }

        // Function key: widths are split by weight across the keyboard width
        const int fnIndex = index - charCount;
        const int n = fnKeyCount();
        const int totalW = kCols * kKeyW + (kCols - 1) * kKeyGap;
        const int avail = totalW - (n - 1) * kKeyGap;

        int weightSum = 0;
        for (int j = 0; j < n; j++)
        {
            weightSum += fnWeight(fnKeyAt(j));
        }

        int offset = 0;
        for (int j = 0; j < fnIndex; j++)
        {
            offset += avail * fnWeight(fnKeyAt(j)) / weightSum + kKeyGap;
        }

        x = (tft.width() - totalW) / 2 + offset;
        y = kFnY;
        w = avail * fnWeight(fnKeyAt(fnIndex)) / weightSum;
        h = kFnH;
    }

    // ------------------------------------------------------------------
    // Session control
    // ------------------------------------------------------------------

    void OnScreenKeyboard::open(TFT_eSPI &tft, const char *title, const char *initialText,
                                size_t maxLength, bool numericOnly)
    {
        snprintf(_title, sizeof(_title), "%s", title ? title : "");
        snprintf(_text, sizeof(_text), "%s", initialText ? initialText : "");

        const size_t limit = static_cast<size_t>(kMaxTextLength);
        _maxLength = (maxLength > limit) ? limit : maxLength;
        _length = strlen(_text);
        if (_length > _maxLength)
        {
            _length = _maxLength;
            _text[_length] = '\0';
        }

        _numericOnly = numericOnly;
        _layer = numericOnly ? Layer::Numeric : Layer::Lower;
        _selected = 0;
        _open = true;

        drawAll(tft);
    }

    OnScreenKeyboard::Result OnScreenKeyboard::update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed)
    {
        if (!_open)
        {
            return Result::None;
        }

        if (encoderDelta != 0)
        {
            const int count = keyCount();
            const int previous = _selected;

            // Wrap around in both directions, however many steps the
            // encoder moved in one loop iteration.
            _selected = ((_selected + static_cast<int>(encoderDelta)) % count + count) % count;

            if (_selected != previous)
            {
                // Only the two keys whose highlight changed need a redraw
                drawKey(tft, previous);
                drawKey(tft, _selected);
            }
        }

        if (buttonPressed)
        {
            return activate(tft);
        }
        return Result::None;
    }

    OnScreenKeyboard::Result OnScreenKeyboard::activate(TFT_eSPI &tft)
    {
        const int charCount = charKeyCount();

        if (_selected < charCount)
        {
            appendChar(tft, charAt(_selected));
            return Result::None;
        }

        switch (fnKeyAt(_selected - charCount))
        {
        case FnKey::Mode:
            cycleLayer(tft);
            break;
        case FnKey::Space:
            appendChar(tft, ' ');
            break;
        case FnKey::Delete:
            deleteLast(tft);
            break;
        case FnKey::Cancel:
            _open = false;
            return Result::Cancelled;
        case FnKey::Done:
            _open = false;
            return Result::Done;
        }
        return Result::None;
    }

    void OnScreenKeyboard::appendChar(TFT_eSPI &tft, char c)
    {
        if (_length >= _maxLength)
        {
            return; // full - ignore
        }
        _text[_length++] = c;
        _text[_length] = '\0';
        drawTextBox(tft);
        drawHeader(tft);
    }

    void OnScreenKeyboard::deleteLast(TFT_eSPI &tft)
    {
        if (_length == 0)
        {
            return;
        }
        _text[--_length] = '\0';
        drawTextBox(tft);
        drawHeader(tft);
    }

    void OnScreenKeyboard::cycleLayer(TFT_eSPI &tft)
    {
        // Keep the highlight on the Mode key even though the number of
        // character keys changes between layers.
        const int fnIndex = _selected - charKeyCount();

        switch (_layer)
        {
        case Layer::Lower:
            _layer = Layer::Upper;
            break;
        case Layer::Upper:
            _layer = Layer::Symbols;
            break;
        default:
            _layer = Layer::Lower; // from Symbols (Numeric has no Mode key)
            break;
        }

        _selected = charKeyCount() + fnIndex;

        // Layers differ in key count and positions: clear the key area first
        const int top = kKeysTop - 3;
        tft.fillRect(0, top, tft.width(), tft.height() - top, t::kBg);
        drawKeys(tft);
        drawHeader(tft);
    }

    // ------------------------------------------------------------------
    // Drawing
    // ------------------------------------------------------------------

    void OnScreenKeyboard::drawAll(TFT_eSPI &tft)
    {
        tft.fillScreen(t::kBg);

        tft.setTextDatum(MC_DATUM);
        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextColor(t::kCream, t::kBg);
        tft.drawString(_title, tft.width() / 2, kTitleY);

        drawHeader(tft);
        drawTextBox(tft);
        drawKeys(tft);
    }

    void OnScreenKeyboard::drawKeys(TFT_eSPI &tft)
    {
        const int count = keyCount();
        for (int i = 0; i < count; i++)
        {
            drawKey(tft, i);
        }
    }

    void OnScreenKeyboard::drawKey(TFT_eSPI &tft, int index)
    {
        int x, y, w, h;
        keyRect(tft, index, x, y, w, h);

        const int charCount = charKeyCount();
        const bool isFn = (index >= charCount);
        const FnKey fn = isFn ? fnKeyAt(index - charCount) : FnKey::Mode;
        const bool selected = (index == _selected);

        uint16_t fill = t::kCardIdle;
        uint16_t border = t::kLeafDark;
        uint16_t textColor = t::kSage;

        if (isFn && fn == FnKey::Done)
        {
            // Done is the primary action, so it stands out even when idle
            fill = t::kLeafDark;
            border = t::kLeafMid;
            textColor = t::kCream;
        }
        if (selected)
        {
            fill = (isFn && fn == FnKey::Cancel) ? t::kBack
                                                 : ((isFn && fn == FnKey::Done) ? t::kLeafMid : t::kLeafDark);
            border = t::kOchre;
            textColor = t::kCream;
        }

        tft.fillRoundRect(x, y, w, h, kKeyRadius, fill);
        tft.drawRoundRect(x, y, w, h, kKeyRadius, border);
        if (selected)
        {
            // Thicker border for the highlighted key
            tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, kKeyRadius - 1, border);
        }

        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(1);
        tft.setTextColor(textColor, fill);

        if (isFn)
        {
            tft.setTextFont(2);
            tft.drawString(fnLabel(fn, _layer), x + w / 2, y + h / 2);
        }
        else
        {
            const char label[2] = {charAt(index), '\0'};
            tft.setTextFont(4);
            tft.drawString(label, x + w / 2, y + h / 2);
        }
    }

    // Layer name on the left and "typed/max" counter on the right of the title row
    void OnScreenKeyboard::drawHeader(TFT_eSPI &tft)
    {
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(t::kSage, t::kBg);

        // The text padding paints over the previous text, so no flicker
        tft.setTextDatum(ML_DATUM);
        tft.setTextPadding(96);
        tft.drawString(layerName(), 16, kTitleY);

        char counter[16];
        snprintf(counter, sizeof(counter), "%u/%u",
                 static_cast<unsigned>(_length), static_cast<unsigned>(_maxLength));
        tft.setTextDatum(MR_DATUM);
        tft.setTextPadding(72);
        tft.drawString(counter, tft.width() - 16, kTitleY);

        tft.setTextPadding(0);
    }

    void OnScreenKeyboard::drawTextBox(TFT_eSPI &tft)
    {
        tft.fillRoundRect(kBoxX, kBoxY, kBoxW, kBoxH, 8, t::kCardIdle);
        tft.drawRoundRect(kBoxX, kBoxY, kBoxW, kBoxH, 8, t::kLeafMid);

        tft.setTextFont(4);
        tft.setTextSize(1);
        tft.setTextDatum(ML_DATUM);
        tft.setTextColor(t::kCream, t::kCardIdle);

        constexpr int kPadding = 16;
        constexpr int kCursorW = 3;
        const int avail = kBoxW - 2 * kPadding - kCursorW - 4;

        // Long text: show the tail, so what you are typing is always visible
        const char *visible = _text;
        while (*visible != '\0' && tft.textWidth(visible) > avail)
        {
            visible++;
        }

        const int textX = kBoxX + kPadding;
        const int midY = kBoxY + kBoxH / 2;

        int textW = 0;
        if (*visible != '\0')
        {
            tft.drawString(visible, textX, midY);
            textW = tft.textWidth(visible);
        }

        // Cursor
        tft.fillRect(textX + textW + 2, midY - 12, kCursorW, 24, t::kOchre);

        // Marker that the beginning is scrolled out of view
        if (visible != _text)
        {
            tft.setTextFont(2);
            tft.setTextColor(t::kSage, t::kCardIdle);
            tft.drawString("<", kBoxX + 4, midY);
        }
    }
}