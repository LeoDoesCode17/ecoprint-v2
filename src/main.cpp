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
  unsigned long last_main_loop = 0;
  unsigned long last_publish_device_status = 0;

  static TFT_eSPI tft = TFT_eSPI();
  ecoprint_device_t global_device_status = {.state_machine = state_manager::get_state_machine(), .is_active = true};

} // namespace

void setup()
{
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  actuator_manager::initialize();
  network_manager::initialize(); // Handles WiFi + MQTT connection + Subscriptions
  sensor_manager::initialize();
  page_manager::initialize(tft);
}

void loop()
{
  // MUST RUN EVERY ITERATION to handle incoming MQTT packets
  network_manager::conect_or_reconnect();
  network_manager::mqtt_loop();

  // Periodic display and sensor publishing
  unsigned long currentMillis = millis();

  if (currentMillis - last_main_loop >= constant::MAIN_LOOP_INTERVAL_MS)
  {
    page_manager::update();
    last_main_loop = currentMillis;
  }

  if (currentMillis - last_publish_device_status >= constant::PUBLISH_DEVICE_STATUS_INTERVAL_MS)
  {
    global_device_status.state_machine = state_manager::get_state_machine();
    network_manager::publish_device_status(global_device_status);
    last_publish_device_status = currentMillis;
  }
}