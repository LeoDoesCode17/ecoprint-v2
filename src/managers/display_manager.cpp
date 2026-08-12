#include "display_manager.h"
#include "sensors/rotary_encoder.h"
#include "config/pin.h"
#include <TFT_eSPI.h>

namespace
{
    TFT_eSPI tft;

    constexpr uint8_t BUTTON_COUNT = 3;

    struct MenuItem
    {
        const char *label;
        display_manager::ActionCallback callback;
    };

    void defaultAction1() { /* action 1 placeholder */ }
    void defaultAction2() { /* action 2 placeholder */ }
    void defaultAction3() { /* action 3 placeholder */ }

    MenuItem menuItems[BUTTON_COUNT] = {
        {"Action 1", defaultAction1},
        {"Action 2", defaultAction2},
        {"Action 3", defaultAction3},
    };

    // Layout constants
    constexpr int16_t BUTTON_X = 40;
    constexpr int16_t BUTTON_W = 240;
    constexpr int16_t BUTTON_H = 60;
    constexpr int16_t BUTTON_GAP = 20;
    constexpr int16_t BUTTON_START_Y = 90; // <-- shifted down to make room for temp readout

    // --- NEW: temperature readout layout ---
    constexpr int16_t TEMP_X = 40;
    constexpr int16_t TEMP_Y = 20;
    constexpr int16_t TEMP_W = 240;
    constexpr int16_t TEMP_H = 50;

    constexpr uint16_t COLOR_BG = TFT_BLACK;
    constexpr uint16_t COLOR_IDLE_FILL = TFT_DARKGREY;
    constexpr uint16_t COLOR_SELECTED_FILL = TFT_BLUE;
    constexpr uint16_t COLOR_TEXT = TFT_WHITE;
    constexpr uint16_t COLOR_BORDER = TFT_WHITE;
    constexpr uint16_t COLOR_TEMP_FILL = TFT_BLACK; // <-- NEW

    // State
    long lastEncoderPos = 0;
    uint8_t currentSelection = 0;
    uint8_t lastDrawnSelection = 255; // force initial draw

    // --- NEW: temperature dirty-check state ---
    float currentTemperature = NAN;
    float lastDrawnTemperature = NAN;
    constexpr float TEMP_EPSILON = 0.05f; // ignore noise-level float jitter

    int16_t buttonY(uint8_t index)
    {
        return BUTTON_START_Y + index * (BUTTON_H + BUTTON_GAP);
    }

    void drawButton(uint8_t index, bool selected)
    {
        int16_t y = buttonY(index);
        uint16_t fillColor = selected ? COLOR_SELECTED_FILL : COLOR_IDLE_FILL;

        tft.fillRoundRect(BUTTON_X, y, BUTTON_W, BUTTON_H, 8, fillColor);
        tft.drawRoundRect(BUTTON_X, y, BUTTON_W, BUTTON_H, 8, COLOR_BORDER);

        tft.setTextColor(COLOR_TEXT, fillColor);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        tft.drawString(menuItems[index].label, BUTTON_X + BUTTON_W / 2, y + BUTTON_H / 2);
    }

    // --- NEW: draws (or redraws) just the temperature readout ---
    void drawTemperature()
    {
        // Clear only the readout area, not the whole screen
        tft.fillRect(TEMP_X, TEMP_Y, TEMP_W, TEMP_H, COLOR_TEMP_FILL);
        tft.drawRoundRect(TEMP_X, TEMP_Y, TEMP_W, TEMP_H, 8, COLOR_BORDER);

        tft.setTextColor(COLOR_TEXT, COLOR_TEMP_FILL);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);

        if (isnan(currentTemperature))
        {
            tft.drawString("Temp: --.-C", TEMP_X + TEMP_W / 2, TEMP_Y + TEMP_H / 2);
        }
        else
        {
            char buffer[16];
            snprintf(buffer, sizeof(buffer), "Temp: %.1fC", currentTemperature);
            tft.drawString(buffer, TEMP_X + TEMP_W / 2, TEMP_Y + TEMP_H / 2);
        }

        lastDrawnTemperature = currentTemperature;
    }

    // --- NEW: only redraw temperature if it actually changed ---
    void redrawTemperatureIfDirty()
    {
        bool bothNan = isnan(currentTemperature) && isnan(lastDrawnTemperature);
        bool changed = !bothNan &&
                       fabs(currentTemperature - lastDrawnTemperature) > TEMP_EPSILON;

        if (changed || (isnan(lastDrawnTemperature) != isnan(currentTemperature)))
        {
            drawTemperature();
        }
    }

    void drawAllButtons()
    {
        tft.fillScreen(COLOR_BG);
        drawTemperature(); // <-- NEW: draw temp readout on initial paint
        for (uint8_t i = 0; i < BUTTON_COUNT; ++i)
        {
            drawButton(i, i == currentSelection);
        }
        lastDrawnSelection = currentSelection;
    }

    void redrawSelectionDelta()
    {
        if (currentSelection == lastDrawnSelection)
        {
            return;
        }

        if (lastDrawnSelection < BUTTON_COUNT)
        {
            drawButton(lastDrawnSelection, false);
        }
        drawButton(currentSelection, true);

        lastDrawnSelection = currentSelection;
    }

    void handleNavigation()
    {
        long pos = rotary_encoder::getPosition();
        long delta = pos - lastEncoderPos;

        if (delta == 0)
        {
            return;
        }
        lastEncoderPos = pos;

        int newSelection = (static_cast<int>(currentSelection) + static_cast<int>(delta)) % BUTTON_COUNT;
        if (newSelection < 0)
        {
            newSelection += BUTTON_COUNT;
        }

        currentSelection = static_cast<uint8_t>(newSelection);
    }

    void handleClick()
    {
        if (rotary_encoder::wasButtonPressed())
        {
            if (menuItems[currentSelection].callback != nullptr)
            {
                menuItems[currentSelection].callback();
            }
        }
    }
}

namespace display_manager
{
    void initialize()
    {
        rotary_encoder::setPosition(0);
        lastEncoderPos = 0;
        currentSelection = 0;

        tft.init();
        tft.setRotation(1); // landscape; adjust to your panel orientation
        drawAllButtons();
    }

    void update()
    {
        handleNavigation();
        redrawSelectionDelta();
        redrawTemperatureIfDirty(); // <-- NEW
        handleClick();
    }

    void setActionCallback(uint8_t buttonIndex, ActionCallback callback)
    {
        if (buttonIndex < BUTTON_COUNT && callback != nullptr)
        {
            menuItems[buttonIndex].callback = callback;
        }
    }

    // --- NEW ---
    void setTemperature(float temperatureCelsius)
    {
        currentTemperature = temperatureCelsius;
    }
}