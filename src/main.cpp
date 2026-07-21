#include <Arduino.h>
#include "managers/sensor_manager.h"

void setup() {
  Serial.begin(115200);
}

void loop() {
  float temperature_celcius = sensor_manager::thermocouple_temperature_celcius();
  Serial.printf("Thermocouple temperature C: %.2f\n", temperature_celcius);
  delay(1000);
}
