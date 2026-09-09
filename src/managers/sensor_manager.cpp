#include "sensor_manager.h"
#include "sensors/thermocouple.h"
#include "sensors/sht3x.h"
#include "sensors/rotary_encoder.h"
#include "config/pin.h"
#include "config/constants.h"

namespace
{
    static const float SMOOTHING_FACTOR = 0.1;
    static float filtered_water_temperature_value[constant::EMA_SMOOTHING_FACTOR_SIZE] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    static float raw_thermocouple_celcius_value = 0.0f;
    static float _setpoint_temperature = 0;
}
namespace sensor_manager
{
    void initialize()
    {
        sht3x::initialize();
        rotary_encoder::initialize(pin::ROTARY_ENCODER_DT, pin::ROTARY_ENCODER_CLK, pin::ROTARY_ENCODER_SW);

        // initialize filter value
        Serial.println("[SENSOR] Populating thermocouple filtered values");
        for (size_t i = 0; i < constant::EMA_SMOOTHING_FACTOR_SIZE; i++)
        {
            filtered_water_temperature_value[i] = thermocouple::temperature_celcius();
            delay(50);
        }
        Serial.println("[SENSOR] Populate thermocouple filtered values done");
    }
    float sht3x_temperature_celcius()
    {
        return sht3x::temprature_celcius();
    }
    float sht3x_humidity_percent()
    {
        return sht3x::humidity_percent();
    }
    float thermocouple_temperature_celcius()
    {
        return thermocouple::temperature_celcius();
    }
    long getEncoderPosition()
    {
        return rotary_encoder::getPosition();
    }

    void setEncoderPosition(long position)
    {
        rotary_encoder::setPosition(position);
    }

    bool wasEncoderButtonPressed()
    {
        return rotary_encoder::wasButtonPressed();
    }

    float smoothed_thermocouple_temperature_celcius(int idx)
    {
        raw_thermocouple_celcius_value = thermocouple::temperature_celcius() + constant::THERMOCOUPLE_OFFSET;
        Serial.printf("[SENSOR] RAW THERMOCOUPLE IS: %f\n", raw_thermocouple_celcius_value);
        filtered_water_temperature_value[idx] = (raw_thermocouple_celcius_value * constant::EMA_SMOOTHING_FACTOR[idx]) + (filtered_water_temperature_value[idx] * (1 - constant::EMA_SMOOTHING_FACTOR[idx]));
        return filtered_water_temperature_value[idx];
    }

    void set_setpoint_temperature(float temperature)
    {
        _setpoint_temperature = temperature;
    }
    float get_setpoint_temperature()
    {
        return _setpoint_temperature;
    }

}