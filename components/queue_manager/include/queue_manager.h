
#ifndef __QUEUE_MANAGER_H
#define __QUEUE_MANAGER_H

#include "app_types.h"
#include "freertos/FreeRTOS.h"


esp_err_t xQueueManagerInit(void);

esp_err_t xEventPost(const app_event_t event);
esp_err_t xEventReceive(app_event_t *event,TickType_t timeout);

esp_err_t xUartPost(const uart_msg_t *msg);
esp_err_t xUartReceive(uart_msg_t *msg,TickType_t timeout);

esp_err_t xMqttPost(const mqtt_msg_t *msg);
esp_err_t xMqttReceive(mqtt_msg_t *msg,TickType_t timeout);

#endif  