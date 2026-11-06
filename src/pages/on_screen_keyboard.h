#pragma once
#include <TFT_eSPI.h>
#include <stddef.h>
#include <stdint.h>

namespace pages
{
    // Encoder-driven on-screen keyboard. It is NOT a page: a page owns one,
    // calls open() to show it, and forwards update() while it is open.
    //
    //   turn  -> move the highlight through the keys (wraps around)
    //   press -> type the highlighted key / run the highlighted function
    //
    // Function keys: Mode (lowercase -> UPPERCASE -> symbols -> ...), Space,
    // Del, Cancel and Done. In numericOnly mode only the digits plus Del,
    // Cancel and Done are shown.
    //
    // Key order is reading order, with the function keys last. Turning
    // counter-clockwise from the first key therefore lands on Done / Del
    // straight away.
    class OnScreenKeyboard
    {
    public:
        enum class Result
        {
            None,     // still editing
            Done,     // user confirmed - read the result with text()
            Cancelled // user backed out - ignore text()
        };

        // Starts an editing session. `initialText` is copied, so the caller's
        // buffer is never modified. maxLength is clamped to 64 characters.
        void open(TFT_eSPI &tft, const char *title, const char *initialText,
                  size_t maxLength, bool numericOnly);

        // Abandons the session without drawing (e.g. when the page is left)
        void close() { _open = false; }

        bool isOpen() const { return _open; }
        const char *text() const { return _text; }

        // Call every loop while open. Returns Done/Cancelled exactly once,
        // when the session ends; the caller must redraw its own screen then.
        Result update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed);

    private:
        enum
        {
            kMaxTextLength = 64,
            kTitleLength = 32
        };

        enum class Layer : uint8_t
        {
            Lower,
            Upper,
            Symbols,
            Numeric
        };

        enum class FnKey : uint8_t
        {
            Mode,
            Space,
            Delete,
            Cancel,
            Done
        };

        // Layout helpers
        const char *rowChars(int row) const;
        int charKeyCount() const;
        char charAt(int index) const;
        int fnKeyCount() const;
        FnKey fnKeyAt(int fnIndex) const;
        static int fnWeight(FnKey key);
        static const char *fnLabel(FnKey key, Layer layer);
        const char *layerName() const;
        int keyCount() const { return charKeyCount() + fnKeyCount(); }
        void keyRect(TFT_eSPI &tft, int index, int &x, int &y, int &w, int &h) const;

        // Drawing
        void drawAll(TFT_eSPI &tft);
        void drawKeys(TFT_eSPI &tft);
        void drawKey(TFT_eSPI &tft, int index);
        void drawHeader(TFT_eSPI &tft);
        void drawTextBox(TFT_eSPI &tft);

        // Editing
        Result activate(TFT_eSPI &tft);
        void appendChar(TFT_eSPI &tft, char c);
        void deleteLast(TFT_eSPI &tft);
        void cycleLayer(TFT_eSPI &tft);

        bool _open = false;
        bool _numericOnly = false;
        Layer _layer = Layer::Lower;
        int _selected = 0; // index into [chars..., function keys...]
        size_t _length = 0;
        size_t _maxLength = 0;
        char _title[kTitleLength];
        char _text[kMaxTextLength + 1];
    };
}