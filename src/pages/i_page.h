#pragma once
#include <TFT_eSPI.h>

namespace pages
{
    // Common interface every screen implements. The PageManager only ever
    // talks to this interface, so it doesn't need to know how many pages
    // exist or what each one does internally.
    class IPage
    {
    public:
        virtual ~IPage() = default;

        // Called once, right after this page becomes active. Draw the
        // full static screen here.
        virtual void onEnter(TFT_eSPI &tft) = 0;

        // Called once, right before another page becomes active.
        // Optional - override only if you need to clean something up.
        virtual void onExit() {}

        // Called every loop() iteration while this page is active.
        // encoderDelta: how much the encoder moved since the last call.
        //   The encoder count is reset to 0 whenever a page becomes
        //   active, so this is always relative to entering the page.
        // buttonPressed: true for exactly one call, on the debounced
        //   press edge.
        virtual void update(TFT_eSPI &tft, long encoderDelta, bool buttonPressed) = 0;
    };
}
