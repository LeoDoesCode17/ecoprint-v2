#include <Arduino.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include "managers/sensor_manager.h"
#include "managers/actuator_manager.h"
#include "managers/network_manager.h"
#include "managers/page_manager.h"
#include "managers/state_manager.h"
#include "managers/queue_manager.h"
#include "config/constants.h"
#include "config/type.h"

namespace
{
  unsigned long last_main_loop = 0;
  unsigned long last_publish_device_status = 0;

  static TFT_eSPI tft = TFT_eSPI();
  ecoprint_device_t global_device_status = {.state_machine = state_manager::get_state_machine(), .is_active = true};

} // namespace

void networkManagementTask(void *pvParameters)
{
  Serial.printf("[RTOS] Core %d - Network Management Task Started.\n", xPortGetCoreID());
  outbound_msg_t msg;
  network_manager::initialize(); // Handles WiFi + MQTT connection + Subscriptions
  for (;;)
  {
    network_manager::conect_or_reconnect();
    network_manager::mqtt_loop();
    if (network_manager::is_connected())
    {
      if (queue_manager::dequeque_outbound(msg, pdMS_TO_TICKS(20)))
      {
        // publish outbound message
        network_manager::publish(msg);
      }
    }
  }
}

void publishDeviceStatusTask(void *pv)
{
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriod = pdMS_TO_TICKS(10000);
  device_status_t device_status = {.state_machine = StateMachine::IDLE};
  Serial.printf("[RTOS] Core %d - publish device status task started.\n", xPortGetCoreID());
  for (;;)
  {
    queue_manager::enqueue_device_status_data(device_status);
    vTaskDelayUntil(&xLastWakeTime, xPeriod);
  }
}

void mainManagementTask(void *pvParameters)
{
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriod = pdMS_TO_TICKS(10);

  Serial.printf("[RTOS] Core %d - Main Management Task Started.\n", xPortGetCoreID());
  for (;;)
  {
    page_manager::update();
    vTaskDelayUntil(&xLastWakeTime, xPeriod);
  }
}

void setup()
{
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  actuator_manager::initialize();
  sensor_manager::initialize();
  page_manager::initialize(tft);
  queue_manager::initialize();

  xTaskCreatePinnedToCore(
      networkManagementTask, // Task Code (function)
      "NetworkTask",         // Task name
      4096,                  // stack depth
      NULL,                  // pvParameters
      1,                     // Task priority (matter for all tasks in the same core),
      NULL,                  // pv created task
      0                      // core ID
  );

  xTaskCreatePinnedToCore(
      mainManagementTask,
      "MainTask",
      4096,
      NULL,
      1,
      NULL,
      1);

  xTaskCreatePinnedToCore(
      publishDeviceStatusTask,
      "PublishDeviceStatusTask",
      4096,
      NULL,
      1,
      NULL,
      1);
}

void loop()
{
  vTaskDelete(NULL);
}