#pragma once
#include <Arduino.h>

namespace display_manager
{
    enum class ProcessStatus
    {
        Persiapan,
        Mengukus,
        STEAMING,
        PREPARATION
    };

    using TargetTemperatureCallback = void (*)(uint8_t targetTempCelsius);
    using ForceStopCallback = void (*)();

    // Wire up encoder pins + TFT, draw the initial screen (main menu)
    void initialize();

    // Call every loop() iteration
    void update();

    // Fired when a temperature button is clicked on the main menu
    // (target temp is already applied internally before this fires)
    void setOnTargetTemperatureSelected(TargetTemperatureCallback callback);

    // Fired when the force stop button is clicked on the sensor screen
    // (state is already reset + screen already switched before this fires)
    void setOnForceStop(ForceStopCallback callback);

    // --- Sensor screen live data setters ---
    void setMeasuredTemperature(float temperatureCelsius);
    void setServoValveState(bool isOn);

    // durationSeconds is only used when status == Mengukus; starts/restarts the countdown
    void setStatus(ProcessStatus status, uint32_t durationSeconds = 0);
}   