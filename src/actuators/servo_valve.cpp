#include "servo_valve.h"
#include <ESP32Servo.h>
#include "config/pin.h"
#include "config/constants.h"

namespace
{
    static Servo servo;
}
namespace servo_valve
{
    void initialize()
    {
        // Allow allocation of all hardware timers for PWM control
        ESP32PWM::allocateTimer(0);
        ESP32PWM::allocateTimer(1);
        ESP32PWM::allocateTimer(2);
        ESP32PWM::allocateTimer(3);

        servo.setPeriodHertz(50); // Standard 50Hz frequency for analog servos

        // Attach the servo with standard microsecond pulse widths for 0 to 180 degrees
        // (Commonly 500us to 2400us/2500us depending on the exact SG90/MG996R variant)
        servo.attach(pin::SERVO_VALVE_PIN, 500, 2400);
        servo.write(constant::ZERO_PERCENT_DEGREE);
    }
    void open_by_degree(unsigned long degree)
    {
        servo.write(degree);
    }
    void close()
    {
        servo.write(constant::ZERO_PERCENT_DEGREE);
    }
}