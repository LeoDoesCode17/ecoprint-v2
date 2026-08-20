// managers/display_manager.cpp
#include "display_manager.h"
#include "sensors/rotary_encoder.h"
#include "config/pin.h"
#include <TFT_eSPI.h>

namespace
{
    TFT_eSPI tft;

    // ---------------------------------------------------------------------
    // Screen state machine
    // ---------------------------------------------------------------------
    enum class Screen
    {
        MainMenu,
        SensorData
    };

    Screen currentScreen = Screen::MainMenu;

    display_manager::TargetTemperatureCallback onTargetTemperatureSelected = nullptr;
    display_manager::ForceStopCallback onForceStop = nullptr;

    // ---------------------------------------------------------------------
    // Shared layout constants
    // ---------------------------------------------------------------------
    constexpr uint16_t COLOR_BG = TFT_BLACK;
    constexpr uint16_t COLOR_IDLE_FILL = TFT_DARKGREY;
    constexpr uint16_t COLOR_SELECTED_FILL = TFT_BLUE;
    constexpr uint16_t COLOR_TEXT = TFT_WHITE;
    constexpr uint16_t COLOR_BORDER = TFT_WHITE;
    constexpr uint16_t COLOR_ROW_FILL = TFT_BLACK;
    constexpr uint16_t COLOR_ON = TFT_GREEN;
    constexpr uint16_t COLOR_OFF = TFT_RED;
    constexpr uint16_t COLOR_STOP_FILL = TFT_RED;

    // Shared encoder tracking
    long lastEncoderPos = 0;

    // ---------------------------------------------------------------------
    // MAIN MENU screen state
    // ---------------------------------------------------------------------
    struct TempButton
    {
        const char *label;
        uint8_t temperatureCelsius;
    };

    constexpr uint8_t MENU_BUTTON_COUNT = 5;
    TempButton tempButtons[MENU_BUTTON_COUNT] = {
        {"80\xF8"
         "C",
         80},
        {"85\xF8"
         "C",
         85},
        {"90\xF8"
         "C",
         90},
        {"95\xF8"
         "C",
         95},
        {"100\xF8"
         "C",
         100},
    };

    constexpr int16_t MENU_BUTTON_X = 40;
    constexpr int16_t MENU_BUTTON_W = 240;
    constexpr int16_t MENU_BUTTON_H = 48;
    constexpr int16_t MENU_BUTTON_GAP = 10;
    constexpr int16_t MENU_BUTTON_START_Y = 20;

    uint8_t menuSelection = 0;
    uint8_t menuLastDrawnSelection = 255; // force initial draw

    int16_t menuButtonY(uint8_t index)
    {
        return MENU_BUTTON_START_Y + index * (MENU_BUTTON_H + MENU_BUTTON_GAP);
    }

    void drawMenuButton(uint8_t index, bool selected)
    {
        int16_t y = menuButtonY(index);
        uint16_t fillColor = selected ? COLOR_SELECTED_FILL : COLOR_IDLE_FILL;

        tft.fillRoundRect(MENU_BUTTON_X, y, MENU_BUTTON_W, MENU_BUTTON_H, 8, fillColor);
        tft.drawRoundRect(MENU_BUTTON_X, y, MENU_BUTTON_W, MENU_BUTTON_H, 8, COLOR_BORDER);

        tft.setTextColor(COLOR_TEXT, fillColor);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        tft.drawString(tempButtons[index].label, MENU_BUTTON_X + MENU_BUTTON_W / 2, y + MENU_BUTTON_H / 2);
    }

    void drawMainMenuFull()
    {
        tft.fillScreen(COLOR_BG);
        for (uint8_t i = 0; i < MENU_BUTTON_COUNT; ++i)
        {
            drawMenuButton(i, i == menuSelection);
        }
        menuLastDrawnSelection = menuSelection;
    }

    void redrawMenuSelectionDelta()
    {
        if (menuSelection == menuLastDrawnSelection)
        {
            return;
        }
        if (menuLastDrawnSelection < MENU_BUTTON_COUNT)
        {
            drawMenuButton(menuLastDrawnSelection, false);
        }
        drawMenuButton(menuSelection, true);
        menuLastDrawnSelection = menuSelection;
    }

    // ---------------------------------------------------------------------
    // SENSOR DATA screen state
    // ---------------------------------------------------------------------
    constexpr int16_t ROW_X = 20;
    constexpr int16_t ROW_W = 440;
    constexpr int16_t ROW_H = 40;
    constexpr int16_t ROW_GAP = 6;
    constexpr int16_t ROW_START_Y = 10;
    constexpr int16_t LABEL_OFFSET_X = 10;
    constexpr int16_t VALUE_OFFSET_X = 250;

    constexpr int16_t STOP_BUTTON_H = 44;

    uint8_t targetTemperatureCelsius = 0;
    float measuredTemperatureCelsius = NAN;
    bool servoValveOn = false;
    display_manager::ProcessStatus processStatus = display_manager::ProcessStatus::PREPARATION;

    uint32_t remainingSeconds = 0;
    unsigned long lastTimerTickMs = 0;

    // Last-drawn values, for dirty-checking each row independently
    uint8_t lastDrawnTargetTemp = 255;
    float lastDrawnMeasuredTemp = NAN;
    bool lastDrawnServoState = true;                                                           // force redraw first pass
    display_manager::ProcessStatus lastDrawnStatus = display_manager::ProcessStatus::PREPARATION; // force redraw first pass
    uint32_t lastDrawnRemainingSeconds = 0xFFFFFFFF;

    constexpr float TEMP_EPSILON = 0.05f;

    int16_t rowY(uint8_t index)
    {
        return ROW_START_Y + index * (ROW_H + ROW_GAP);
    }

    void clearRow(int16_t y)
    {
        tft.fillRect(ROW_X, y, ROW_W, ROW_H, COLOR_ROW_FILL);
        tft.drawRoundRect(ROW_X, y, ROW_W, ROW_H, 6, COLOR_BORDER);
    }

    void drawRowLabel(int16_t y, const char *label)
    {
        tft.setTextColor(COLOR_TEXT, COLOR_ROW_FILL);
        tft.setTextDatum(ML_DATUM); // middle-left
        tft.setTextSize(2);
        tft.drawString(label, ROW_X + LABEL_OFFSET_X, y + ROW_H / 2);
    }

    // Row 0: Suhu target
    void drawTargetTempRow()
    {
        int16_t y = rowY(0);
        clearRow(y);
        drawRowLabel(y, "Suhu target");

        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d\xF8"
                                         "C",
                 targetTemperatureCelsius);

        tft.setTextColor(COLOR_TEXT, COLOR_ROW_FILL);
        tft.setTextDatum(ML_DATUM);
        tft.setTextSize(2);
        tft.drawString(buffer, ROW_X + VALUE_OFFSET_X, y + ROW_H / 2);

        lastDrawnTargetTemp = targetTemperatureCelsius;
    }

    // Row 1: Suhu terukur
    void drawMeasuredTempRow()
    {
        int16_t y = rowY(1);
        clearRow(y);
        drawRowLabel(y, "Suhu terukur");

        char buffer[16];
        if (isnan(measuredTemperatureCelsius))
        {
            snprintf(buffer, sizeof(buffer), "--.-\xF8"
                                             "C");
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "%.1f\xF8"
                                             "C",
                     measuredTemperatureCelsius);
        }

        tft.setTextColor(COLOR_TEXT, COLOR_ROW_FILL);
        tft.setTextDatum(ML_DATUM);
        tft.setTextSize(2);
        tft.drawString(buffer, ROW_X + VALUE_OFFSET_X, y + ROW_H / 2);

        lastDrawnMeasuredTemp = measuredTemperatureCelsius;
    }

    // Row 2: Servo valve
    void drawServoValveRow()
    {
        int16_t y = rowY(2);
        clearRow(y);
        drawRowLabel(y, "Servo valve");

        uint16_t stateColor = servoValveOn ? COLOR_ON : COLOR_OFF;
        tft.setTextColor(stateColor, COLOR_ROW_FILL);
        tft.setTextDatum(ML_DATUM);
        tft.setTextSize(2);
        tft.drawString(servoValveOn ? "ON" : "OFF", ROW_X + VALUE_OFFSET_X, y + ROW_H / 2);

        lastDrawnServoState = servoValveOn;
    }

    // Row 3: Status
    void drawStatusRow()
    {
        int16_t y = rowY(3);
        clearRow(y);
        drawRowLabel(y, "Status");

        const char *statusText = (processStatus == display_manager::ProcessStatus::PREPARATION)
                                     ? "Persiapan"
                                     : "Mengukus";

        tft.setTextColor(COLOR_TEXT, COLOR_ROW_FILL);
        tft.setTextDatum(ML_DATUM);
        tft.setTextSize(2);
        tft.drawString(statusText, ROW_X + VALUE_OFFSET_X, y + ROW_H / 2);

        lastDrawnStatus = processStatus;
    }

    // Row 4: Timer
    void drawTimerRow()
    {
        int16_t y = rowY(4);
        clearRow(y);
        drawRowLabel(y, "Timer");

        uint32_t mins = remainingSeconds / 60;
        uint32_t secs = remainingSeconds % 60;
        char buffer[8];
        snprintf(buffer, sizeof(buffer), "%02u:%02u", (unsigned)mins, (unsigned)secs);

        tft.setTextColor(COLOR_TEXT, COLOR_ROW_FILL);
        tft.setTextDatum(ML_DATUM);
        tft.setTextSize(2);
        tft.drawString(buffer, ROW_X + VALUE_OFFSET_X, y + ROW_H / 2);

        lastDrawnRemainingSeconds = remainingSeconds;
    }

    // Row 5: Force stop button
    void drawForceStopButton()
    {
        int16_t y = rowY(5);
        tft.fillRoundRect(ROW_X, y, ROW_W, STOP_BUTTON_H, 8, COLOR_STOP_FILL);
        tft.drawRoundRect(ROW_X, y, ROW_W, STOP_BUTTON_H, 8, COLOR_BORDER);

        tft.setTextColor(COLOR_TEXT, COLOR_STOP_FILL);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        tft.drawString("FORCE STOP", ROW_X + ROW_W / 2, y + STOP_BUTTON_H / 2);
    }

    void drawSensorScreenFull()
    {
        tft.fillScreen(COLOR_BG);
        drawTargetTempRow();
        drawMeasuredTempRow();
        drawServoValveRow();
        drawStatusRow();
        drawTimerRow();
        drawForceStopButton();
    }

    void redrawSensorScreenDirtyFields()
    {
        if (targetTemperatureCelsius != lastDrawnTargetTemp)
        {
            drawTargetTempRow();
        }

        bool bothTempNan = isnan(measuredTemperatureCelsius) && isnan(lastDrawnMeasuredTemp);
        bool tempChanged = !bothTempNan &&
                           (isnan(measuredTemperatureCelsius) != isnan(lastDrawnMeasuredTemp) ||
                            fabs(measuredTemperatureCelsius - lastDrawnMeasuredTemp) > TEMP_EPSILON);
        if (tempChanged)
        {
            drawMeasuredTempRow();
        }

        if (servoValveOn != lastDrawnServoState)
        {
            drawServoValveRow();
        }

        if (processStatus != lastDrawnStatus)
        {
            drawStatusRow();
        }

        if (remainingSeconds != lastDrawnRemainingSeconds)
        {
            drawTimerRow();
        }
    }

    // ---------------------------------------------------------------------
    // Screen transitions
    // ---------------------------------------------------------------------
    void switchToMainMenu()
    {
        currentScreen = Screen::MainMenu;
        menuLastDrawnSelection = 255; // force full redraw on entry
        drawMainMenuFull();
    }

    void switchToSensorScreen()
    {
        currentScreen = Screen::SensorData;
        // Force full redraw of every row on entry
        lastDrawnTargetTemp = 255;
        lastDrawnMeasuredTemp = NAN;
        lastDrawnServoState = !servoValveOn;
        lastDrawnStatus = (processStatus == display_manager::ProcessStatus::PREPARATION)
                              ? display_manager::ProcessStatus::STEAMING
                              : display_manager::ProcessStatus::PREPARATION;
        lastDrawnRemainingSeconds = 0xFFFFFFFF;
        drawSensorScreenFull();
    }

    void resetProcessState()
    {
        targetTemperatureCelsius = 0;
        measuredTemperatureCelsius = NAN;
        servoValveOn = false;
        processStatus = display_manager::ProcessStatus::PREPARATION;
        remainingSeconds = 0;
    }

    // ---------------------------------------------------------------------
    // Input handling
    // ---------------------------------------------------------------------
    void handleMainMenuNavigation()
    {
        long pos = rotary_encoder::getPosition();
        long delta = pos - lastEncoderPos;
        if (delta == 0)
        {
            return;
        }
        lastEncoderPos = pos;

        int newSelection = (static_cast<int>(menuSelection) + static_cast<int>(delta)) % MENU_BUTTON_COUNT;
        if (newSelection < 0)
        {
            newSelection += MENU_BUTTON_COUNT;
        }
        menuSelection = static_cast<uint8_t>(newSelection);
    }

    void handleMainMenuClick()
    {
        if (rotary_encoder::wasButtonPressed())
        {
            targetTemperatureCelsius = tempButtons[menuSelection].temperatureCelsius;
            switchToSensorScreen();

            if (onTargetTemperatureSelected != nullptr)
            {
                onTargetTemperatureSelected(targetTemperatureCelsius);
            }
        }
    }

    void handleSensorScreenClick()
    {
        // Sensor screen has a single interactive element (Force Stop).
        // Rotation is intentionally ignored here; any click triggers it.
        if (rotary_encoder::wasButtonPressed())
        {
            resetProcessState();
            switchToMainMenu();

            if (onForceStop != nullptr)
            {
                onForceStop();
            }
        }
    }

    void handleSensorScreenTimer()
    {
        if (processStatus != display_manager::ProcessStatus::STEAMING)
        {
            return;
        }
        if (remainingSeconds == 0)
        {
            return;
        }

        unsigned long now = millis();
        if (now - lastTimerTickMs >= 1000)
        {
            lastTimerTickMs = now;
            remainingSeconds -= 1;
        }
    }
}

namespace display_manager
{
    void initialize()
    {
        rotary_encoder::setPosition(0);
        lastEncoderPos = 0;

        tft.init();
        tft.setRotation(1); // landscape; adjust to your panel orientation

        resetProcessState();
        switchToMainMenu();
    }

    void update()
    {
        if (currentScreen == Screen::MainMenu)
        {
            handleMainMenuNavigation();
            redrawMenuSelectionDelta();
            handleMainMenuClick();
        }
        else // Screen::SensorData
        {
            handleSensorScreenTimer();
            redrawSensorScreenDirtyFields();
            handleSensorScreenClick();
        }
    }

    void setOnTargetTemperatureSelected(TargetTemperatureCallback callback)
    {
        onTargetTemperatureSelected = callback;
    }

    void setOnForceStop(ForceStopCallback callback)
    {
        onForceStop = callback;
    }

    void setMeasuredTemperature(float temperatureCelsius)
    {
        measuredTemperatureCelsius = temperatureCelsius;
    }

    void setServoValveState(bool isOn)
    {
        servoValveOn = isOn;
    }

    void setStatus(ProcessStatus status, uint32_t durationSeconds)
    {
        bool enteringSteaming = (status == ProcessStatus::STEAMING &&
                                 processStatus != ProcessStatus::STEAMING);

        processStatus = status;

        if (enteringSteaming)
        {
            remainingSeconds = durationSeconds;
            lastTimerTickMs = millis();
        }
        else if (status == ProcessStatus::PREPARATION)
        {
            remainingSeconds = 0;
        }
    }
}