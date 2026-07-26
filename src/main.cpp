#include <Arduino.h>
#include "managers/sensor_manager.h"
#include "managers/actuator_manager.h"

void setup() {
  Serial.begin(115200);
  actuator_manager::initialize();
  actuator_manager::open_valve_by_percent(0.25f);
  delay(2000);
  actuator_manager::open_valve_by_percent(0.5f);
  delay(2000);
  actuator_manager::open_valve_by_percent(0.75f);
  delay(2000);
  actuator_manager::open_valve_by_percent(1.0f);
  delay(2000);
  actuator_manager::open_valve_by_percent(0.0f);
}

void loop() {
  // float temperature_celcius = sensor_manager::thermocouple_temperature_celcius();
  // Serial.printf("Thermocouple temperature C: %.2f\n", temperature_celcius);
  // delay(1000);
}
