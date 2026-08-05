#include <Arduino.h>
#include <ArduinoJson.h>
#include "managers/sensor_manager.h"
#include "managers/actuator_manager.h"
#include "managers/network_manager.h"
#include "managers/display_manager.h"
#include "config/constants.h"
#include "config/type.h"

namespace
{
  unsigned long last_main_loop = millis();
  unsigned long last_publish_sensor_data = millis();
  unsigned long last_publish_device_status = millis();

  ecoprint_device_t global_device_status = {.state_machine = IDLE, .is_active = true};
  ecoprint_sensor_t global_sensor_data = {.water_temperature = 0.0f, .air_temperature = 0.5f, .air_humidity = 90.5f, .is_water_sufficient = true, .event = EcoprintEvent::PREPARATION};

  float global_constant_temperature = 0.0f;
  unsigned long global_steaming_time = 7200000;

  static bool sensorScreenShown = false;

  static void on_mqtt_message(char *topic, byte *payload, unsigned int length)
  {
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err)
    {
      Serial.printf("[MQTT] JSON parse failed: %s\n", err.c_str());
      return;
    }
    if (strcmp(topic, constant::SUBSCRIBE_ACTUATOR_TOPIC) == 0)
    {
      const char *actuator = doc["actuator"];
      const int value = doc["value"];
      if (!actuator)
      {
        Serial.println("[MQTT] Missing required fields");
        return;
      }
      if (strcmp(actuator, "servo_valve") == 0)
      {
        actuator_manager::open_valve_by_percent(value);
        Serial.printf("[ACTUATOR] Opening servo valve by %d%\n", value);
      }
    }
    else if (strcmp(topic, constant::ECOPRINT_SUBSCRIBE_COMMAND_TOPIC) == 0)
    {
      const char *event = doc["event"];
      if (!event)
      {
        Serial.println("[MQTT] Missing 'event' field");
        return;
      }

      if (strcmp(event, "session_start") == 0)
      {
        const char *fabricType = doc["fabric_type"];
        float boilingTemp = doc["boiling_temp"] | 0.0f; // 0.0f = default if missing
        global_constant_temperature = boilingTemp;

        if (!fabricType)
        {
          Serial.println("[MQTT] session_start: missing fabric_type");
          return;
        }

        Serial.printf("[MQTT] session_start — fabric: %s  temp: %.1f C\n",
                      fabricType, boilingTemp);
        global_device_status.state_machine = PREPARATION;
      }
      else if (strcmp(event, "session_stop") == 0)
      {
        Serial.println("[MQTT] session_stop received");
        actuator_manager::close_valve();
      }
      else
      {
        Serial.printf("[MQTT] Unknown event: %s\n", event);
      }
    }
    else
    {
      // TODO: IMPLEMENT IF OTHER TOPIC RECEIVED
    }
  }

  static const char *event_to_string(EcoprintEvent event)
  {
    switch (event)
    {
    case EcoprintEvent::PREPARATION:
      return "preparation";
    case EcoprintEvent::STEAMING:
      return "steaming";
    default:
      return "unknown";
    }
  }
} // namespace

void setup()
{
  Serial.begin(115200);
  actuator_manager::initialize();
  network_manager::initialize();
  network_manager::set_mqtt_callback(on_mqtt_message);
  sensor_manager::initialize();
  display_manager::initialize();
}

void loop()
{
  if (millis() - last_main_loop >= constant::MAIN_LOOP_INTERVAL_MS)
  {
    network_manager::mqtt_loop();
    network_manager::conect_or_reconnect();
    long encoderPosition = sensor_manager::getEncoderPosition();
    bool buttonPressed = sensor_manager::wasEncoderButtonPressed();

    display_manager::update(encoderPosition, buttonPressed);

    if (millis() - last_publish_device_status >= constant::PUBLISH_DEVICE_STATUS_INTERVAL_MS)
    {
      network_manager::publish_device_status(global_device_status);
      last_publish_device_status = millis();
    }

    if (millis() - last_publish_sensor_data >= constant::PUBLISH_SENSOR_DATA_INTERVAL_MS)
    {
      global_sensor_data.water_temperature = sensor_manager::thermocouple_temperature_celcius();
      global_sensor_data.air_temperature = sensor_manager::sht3x_temperature_celcius();
      global_sensor_data.air_humidity = sensor_manager::sht3x_humidity_percent();
      network_manager::publish_sensor_data(global_sensor_data);
      last_publish_sensor_data = millis();
    }

    if (global_sensor_data.water_temperature >= global_constant_temperature)
      global_device_status.state_machine = STEAMING;

    if ((global_device_status.state_machine == PREPARATION ||
         global_device_status.state_machine == STEAMING) &&
        !sensorScreenShown)
    {
      display_manager::showSensorScreen(global_sensor_data);
      sensorScreenShown = true;
    }

    // Optionally, when IDLE, allow returning to main menu:
    if (global_device_status.state_machine == IDLE)
    {
      sensorScreenShown = false;
      // If you want to force main menu here, you can add:
      // display_manager::initialize();  // or a dedicated showMainMenu() function
    }

    last_main_loop = millis();
  }
}
