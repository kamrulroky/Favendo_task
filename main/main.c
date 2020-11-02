//WiFi and MQTT connection (Favendo Task )

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "connect.h"
#include "mqtt_client.h"

#define TAG "MQTT"

TaskHandle_t taskHandle;

//BITS for Wifi and MQTT
const uint32_t WIFI_CONNEECTED = BIT1;
const uint32_t MQTT_CONNECTED = BIT2;
const uint32_t MQTT_PUBLISHED = BIT3;

uint32_t messageCounter = 0;
char  MQTTmessage[20];

//setting-up event handler
void mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
  switch (event->event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    xTaskNotify(taskHandle, MQTT_CONNECTED, eSetValueWithOverwrite);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    break;
  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    xTaskNotify(taskHandle, MQTT_PUBLISHED, eSetValueWithOverwrite);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    printf("DATA=%.*s\r\n", event->data_len, event->data);
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    break;
  default:
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }
}

//initialize event handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  mqtt_event_handler_cb(event_data);
}

void MQTTLogic(const char *message)
{
  uint32_t command = 0;
  //configure mqtt credentials 
  esp_mqtt_client_config_t mqttConfig = {
      .uri = "mqtt://staging.mqtt.espdoor.fvndo.net:1883",
      .username = "ex_0",
      .password = "H91MRHAKeeWLOg94ZZmbDVCwfZcnsn0BgzECt2Q3E6g=",
      .client_id = "aDeviceId"
      };
  esp_mqtt_client_handle_t client = NULL;

  while (true)
  {
    //task for waiting for broker connection notification 
    xTaskNotifyWait(0, 0, &command, portMAX_DELAY);
    switch (command)
    {
    case WIFI_CONNEECTED:
      client = esp_mqtt_client_init(&mqttConfig);
      esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
      esp_mqtt_client_start(client);
      break;
    case MQTT_CONNECTED:
      printf("sending data: %s" , message);
      esp_mqtt_client_publish(client, "f/i/e/aDeviceId/ex_0/aTopic", message, strlen(message), 2, false);
      break;
      //After each publish clears on connection event
    case MQTT_PUBLISHED:
      esp_mqtt_client_stop(client);
      esp_mqtt_client_destroy(client);
      esp_wifi_stop();
      return;
    default:
      break;
    }
  }
}
//sends a string to the broker
void OnConnected(void *para)
{
  while (true)
  {
    messageCounter++;
    snprintf(MQTTmessage,20,"aTestMessage %d",messageCounter);
    ESP_ERROR_CHECK(esp_wifi_start());
    MQTTLogic(MQTTmessage);
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}


void app_main()
{
  wifiInit();
  xTaskCreate(OnConnected, "On MQTT Connection", 1024 * 5, NULL, 5, &taskHandle);
}