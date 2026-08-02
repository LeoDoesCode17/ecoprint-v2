#include <Arduino.h>
#include "managers/sensor_manager.h"
#include "managers/actuator_manager.h"
#include "managers/network_manager.h"
#include "config/constants.h"
#include "config/type.h"

unsigned long last_main_loop = millis();
unsigned long last_publish_sensor_data = millis();
unsigned long last_publish_device_status = millis();

ecoprint_device_t global_device_status = {.state_machine = IDLE, .is_active = true};
ecoprint_sensor_t global_sensor_data = {.water_temperature = 0.0f, .air_temperature = 0.5f, .air_humidity = 90.5f, .is_water_sufficient = true};

void setup()
{
  Serial.begin(115200);
  actuator_manager::initialize();
  network_manager::initialize();
}

void loop()
{
  if (millis() - last_main_loop >= constant::MAIN_LOOP_INTERVAL_MS)
  {
    network_manager::mqtt_loop();
    network_manager::conect_or_reconnect();

    if (millis() - last_publish_device_status >= constant::PUBLISH_DEVICE_STATUS_INTERVAL_MS)
    {
      network_manager::publish_device_status(global_device_status);
      last_publish_device_status = millis();
    }

    if (millis() - last_publish_sensor_data >= constant::PUBLISH_SENSOR_DATA_INTERVAL_MS)
    {
      network_manager::publish_sensor_data(global_sensor_data);
      last_publish_sensor_data = millis();
    }

    last_main_loop = millis();
  }
  // float temperature_celcius = sensor_manager::thermocouple_temperature_celcius();
  // Serial.printf("Thermocouple temperature C: %.2f\n", temperature_celcius);
  // delay(1000);
}
