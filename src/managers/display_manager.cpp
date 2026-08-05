#include "display_manager.h"
#include <TFT_eSPI.h>

// If you have a display_config, keep using it. If not, see the alternate
// color/size definitions below (commented).
#include "config/display.h"

namespace
{
    TFT_eSPI tft;

    enum class Screen : uint8_t
    {
        MAIN,
        SENSOR
    };

    Screen currentScreen = Screen::MAIN;

    // Simple button for MAIN menu
    struct ButtonDef
    {
        const char *label;
        int16_t x, y, w, h;
    };

    ButtonDef sensorButton;

    // Back button (used on SENSOR screen)
    ButtonDef backButton;

    // -----------------------------------------------------------------
    // Layout helpers
    // -----------------------------------------------------------------
    void computeSensorButtonLayout()
    {
        sensorButton.label = "SENSOR";
        sensorButton.w = 200;
        sensorButton.h = 60;
        sensorButton.x = (tft.width() - sensorButton.w) / 2;
        sensorButton.y = 120;
    }

    void computeBackButtonLayout()
    {
        backButton.label = "BACK";
        backButton.w = 120;
        backButton.h = 50;
        backButton.x = (tft.width() - backButton.w) / 2;
        backButton.y = tft.height() - backButton.h - 20;
    }

    // -----------------------------------------------------------------
    // Drawing helpers
    // -----------------------------------------------------------------
    void drawButton(const ButtonDef &btn, uint16_t fillColor, uint16_t textColor)
    {
        tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 10, fillColor);
        tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 10, TFT_WHITE);

        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(textColor, fillColor);
        tft.setTextSize(2);
        tft.drawString(btn.label, btn.x + btn.w / 2, btn.y + btn.h / 2);
    }

    void drawMainScreen()
    {
        tft.fillScreen(TFT_BLACK);

        // Title
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setTextSize(3);
        tft.drawString("ECOPRINT", tft.width() / 2, 40);

        drawButton(sensorButton, TFT_BLUE, TFT_WHITE);
    }

    void drawSensorScreen(const ecoprint_sensor_t &data)
    {
        tft.fillScreen(TFT_BLACK);

        // Title
        tft.setTextDatum(TC_DATUM);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setTextSize(2);
        tft.drawString("SENSOR DATA", tft.width() / 2, 30);

        const int16_t cx = tft.width() / 2;
        int16_t y = 70;

        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(2);

        auto drawLine = [&](const char *label, const char *value) {
            tft.setTextDatum(ML_DATUM);
            tft.drawString(label, cx - 10, y);
            tft.setTextDatum(MR_DATUM);
            tft.drawString(value, cx + 10, y);
            y += tft.fontHeight() + 8;
        };

        char buf[16];

        snprintf(buf, sizeof(buf), "%.1f C", data.water_temperature);
        drawLine("Water Temp:", buf);

        snprintf(buf, sizeof(buf), "%.1f C", data.air_temperature);
        drawLine("Air Temp:", buf);

        snprintf(buf, sizeof(buf), "%.1f %%", data.air_humidity);
        drawLine("Humidity:", buf);

        drawLine("Water:", data.is_water_sufficient ? "OK" : "LOW");

        const char *eventStr = "UNKNOWN";
        switch (data.event)
        {
        case EcoprintEvent::PREPARATION:
            eventStr = "PREPARATION";
            break;
        case EcoprintEvent::STEAMING:
            eventStr = "STEAMING";
            break;
        }
        drawLine("Event:", eventStr);

        // Back button
        drawButton(backButton, TFT_DARKGREY, TFT_WHITE);
    }

    // -----------------------------------------------------------------
    // Navigation
    // -----------------------------------------------------------------
    bool hitTest(const ButtonDef &btn, long encoderPosition)
    {
        // For now, we treat encoderPosition as a simple index:
        // 0 = first button, 1 = second, etc.
        // You can adapt this to your own selection logic.
        return false; // not used in this minimal version
    }

    void enterMainScreen()
    {
        currentScreen = Screen::MAIN;
        drawMainScreen();
    }

    void enterSensorScreen(const ecoprint_sensor_t &data)
    {
        currentScreen = Screen::SENSOR;
        drawSensorScreen(data);
    }
} // namespace

namespace display_manager
{
    void initialize()
    {
        tft.init();
        tft.setRotation(1); // adjust as needed

        computeSensorButtonLayout();
        computeBackButtonLayout();

        enterMainScreen();
    }

    void update(long encoderPosition, bool buttonPressed)
    {
        // Minimal logic:
        // - If on MAIN and button pressed -> go to SENSOR with dummy data
        // - If on SENSOR and button pressed -> go back to MAIN
        //
        // In your real app, you’ll use encoderPosition to move selection
        // and call showSensorScreen() from main.cpp instead of here.

        static bool lastButton = false;
        bool edge = buttonPressed && !lastButton;
        lastButton = buttonPressed;

        if (!edge)
            return;

        if (currentScreen == Screen::MAIN)
        {
            // For now, just jump to sensor screen with dummy data.
            // In your final version, you probably won't use this path
            // and will call showSensorScreen() from main.cpp directly.
            // ecoprint_sensor_t dummy{};
            // dummy.water_temperature = 25.0f;
            // dummy.air_temperature = 30.0f;
            // dummy.air_humidity = 60.0f;
            // dummy.is_water_sufficient = true;
            // dummy.event = EcoprintEvent::PREPARATION;

            // enterSensorScreen(dummy);
        }
        else if (currentScreen == Screen::SENSOR)
        {
            enterMainScreen();
        }
    }

    void showSensorScreen(const ecoprint_sensor_t &data)
    {
        enterSensorScreen(data);
    }
}