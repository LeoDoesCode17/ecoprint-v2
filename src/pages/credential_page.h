#pragma once
#include <stddef.h>
#include "i_page.h"
#include "page_id.h"
#include "on_screen_keyboard.h"

namespace pages
{
    // One editable credential. The page never stores the value itself: it
    // reads it with get() and hands the new value to set() when the user
    // confirms the keyboard.
    struct CredentialField
    {
        const char *label;
        bool masked;      // show '*' instead of the value on the card
        bool numericOnly; // keyboard offers digits only (e.g. port)
        size_t maxLength; // max characters, without the terminator
        const char *(*get)();
        void (*set)(const char *);
        // Optional. Return an error message (string literal) to reject the
        // value, or nullptr to accept it. May be nullptr.
        const char *(*validate)(const char *);
    };

    // Carousel page (same style as the other pages): one card per field,
    // plus a Back card.
    //
    //   Field card: press -> opens the on-screen keyboard
    //               Done  -> validate + set() + "Saved" hint
    //               Cancel-> nothing changes
    //   Back card:  press -> navigates to backTarget
    //
    // WifiConfigurationPage and MqttConfigurationPage only supply the fields.
    class CredentialPage : public IPage
    {
    public:
        CredentialPage(const char *title, const CredentialField *fields,
                       int fieldCount, PageId backTarget);

        void onEnter(TFT_eSPI &tft) override;
        void onExit() override;
        void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) override;

    private:
        // fields + 1 Back card
        int itemCount() const { return _fieldCount + 1; }
        int wrapIndex(int i) const;

        void drawScreen(TFT_eSPI &tft);
        void drawCarousel(TFT_eSPI &tft);
        void drawCard(TFT_eSPI &tft, int itemIndex, int x, bool selected);
        void drawDots(TFT_eSPI &tft);
        void drawHint(TFT_eSPI &tft);
        void finishEditing(TFT_eSPI &tft, OnScreenKeyboard::Result result);

        const char *_title;
        const CredentialField *_fields;
        int _fieldCount;
        PageId _backTarget;

        int _selectedIndex = 0;
        int _editingIndex = -1; // field being edited while the keyboard is open

        // One-line feedback shown in the hint area ("Saved" / error)
        const char *_message = nullptr;
        bool _messageIsError = false;

        OnScreenKeyboard _keyboard;
    };
}