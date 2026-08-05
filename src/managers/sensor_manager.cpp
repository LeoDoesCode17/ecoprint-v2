#include "sensor_manager.h"
#include "sensors/thermocouple.h"
#include "sensors/sht3x.h"
#include "sensors/rotary_encoder.h"
#include "config/pin.h"

namespace sensor_manager
{
    void initialize()
    {
        sht3x::initialize();
        rotary_encoder::initialize(pin::ROTARY_ENCODER_DT, pin::ROTARY_ENCODER_CLK, pin::ROTARY_ENCODER_SW);
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
}