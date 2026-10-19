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

  // FreeRTOS handle
  QueueHandle_t mqttPublishQueue;
  const int QUEUE_LENGTH = 5;
  const int QUEUE_MESSAGE_BUFFER_SIZE = 50;

} // namespace

void networkManagementTask(void *pvParameters)
{
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriod = pdMS_TO_TICKS(10);
  Serial.printf("[RTOS] Core %d - Network Management Task Started.\n", xPortGetCoreID());
  // network initialization must be non blocking task
  network_manager::initialize(); // Handles WiFi + MQTT connection + Subscriptions
  for (;;)
  {
    network_manager::conect_or_reconnect();
    network_manager::mqtt_loop();
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

void publishDeviceStatusManagementTask(void *pvParameters)
{
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xPeriod = pdMS_TO_TICKS(10000);

  Serial.printf("[RTOS] Core %d - Publish Device Status Management Task Started.\n", xPortGetCoreID());
  for (;;)
  {
    // publish status device
    // global_device_status.state_machine = state_manager::get_state_machine();
    // network_manager::publish_device_status(global_device_status);
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

  // mqtt publish queue
  mqttPublishQueue = xQueueCreate(QUEUE_LENGTH, sizeof(char) * QUEUE_MESSAGE_BUFFER_SIZE);
  if (mqttPublishQueue == NULL)
  {
    Serial.println("[RTOS] Error creating the FreeRTOS Queue");
    while (1)
      ; // halt system for now
  }

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
      publishDeviceStatusManagementTask,
      "PublishStatusDeviceTask",
      4096,
      NULL,
      1,
      NULL,
      1);
}

void loop()
{
}