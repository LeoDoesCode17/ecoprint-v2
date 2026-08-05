#include "sht3x.h"
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include "config/pin.h"
#include "config/constants.h"

namespace
{
    static Adafruit_SHT31 sht31 = Adafruit_SHT31();
}
namespace sht3x
{
    void initialize()
    {
        Wire.begin(pin::SDA_PIN, pin::SCL_PIN);
        Serial.println("[SENSOR] SHT3X INIT");
        if (!sht31.begin(constant::SHT3X_ADDR))
        {
            Serial.println("Couldn't find SHT31");
            // while (1) delay(1);
        }
    }
    float temprature_celcius()
    {
        float temperature = sht31.readTemperature();
        // if (!isnan(temperature))
        // { // check if 'is not a number'
        //     Serial.print("Temp *C = ");
        //     Serial.print(temperature);
        //     Serial.print("\t\t");
        // }
        // else
        // {
        //     temperature = -99.9;
        //     Serial.println("Failed to read temperature");
        // }
        if (isnan(temperature))  temperature = -99.99;
        return temperature;
    }
    float humidity_percent()
    {
        float humidity = sht31.readHumidity();
        // if (!isnan(humidity))
        // { // check if 'is not a number'
        //     Serial.print("Humidity % = ");
        //     Serial.print(humidity);
        //     Serial.print("\t\t");
        // }
        // else
        // {
        //     humidity = -99.9;
        //     Serial.println("Failed to read humidity");
        // }
        if (isnan(humidity)) humidity = -99.99;
        return humidity;
    }
}