#include <Arduino.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "managers/sensor_manager.h"
#include "managers/actuator_manager.h"
#include "managers/network_manager.h"
#include "managers/page_manager.h"
#include "managers/state_manager.h"
#include "config/constants.h"
#include "config/type.h"

namespace
{
  unsigned long last_main_loop = millis();
  unsigned long last_publish_sensor_data = millis();
  unsigned long last_publish_device_status = millis();
  unsigned long last_update_sensor_data = millis();

  // Scope global
  static TFT_eSPI tft = TFT_eSPI();
  ecoprint_device_t global_device_status = {.state_machine = state_manager::get_state_machine(), .is_active = true};

} // namespace

void setup()
{
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1); // landscape - adjust to match your wiring/orientation
  tft.fillScreen(TFT_BLACK);

  actuator_manager::initialize();
  network_manager::initialize();
  sensor_manager::initialize();
  page_manager::initialize(tft);
}

void loop()
{
  if (millis() - last_main_loop >= constant::MAIN_LOOP_INTERVAL_MS)
  {
    page_manager::update();
    network_manager::mqtt_loop();
    network_manager::conect_or_reconnect();

    if (millis() - last_publish_device_status >= constant::PUBLISH_DEVICE_STATUS_INTERVAL_MS)
    {
      // update global_device_status
      global_device_status.state_machine = state_manager::get_state_machine();
      network_manager::publish_device_status(global_device_status); // see more
      last_publish_device_status = millis();
    }
    last_main_loop = millis();
  }
}
