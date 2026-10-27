

#include "queue_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* private variables */
static QueueHandle_t xEventQueue;
static QueueHandle_t xUartQueue;
static QueueHandle_t xMqttQueue;

/* Initialization */
esp_err_t xQueueManagerInit(void)
{
    xEventQueue = xQueueCreate(16,sizeof(app_event_t));
    xUartQueue  = xQueueCreate(10,sizeof(uart_msg_t));
    xMqttQueue  = xQueueCreate(10,sizeof(mqtt_msg_t));

    configASSERT(xEventQueue);
    configASSERT(xUartQueue);
    configASSERT(xMqttQueue);

    return ESP_OK;
}


/* Methods */
esp_err_t xEventPost(const app_event_t event){

    return xQueueSend(
        xEventQueue,
        &event,
        0) == pdPASS ? ESP_OK : ESP_ERR_TIMEOUT;

}

esp_err_t xEventReceive(app_event_t *event,TickType_t timeout){

    return xQueueReceive(
        xEventQueue,
        event,
        timeout) == pdPASS ? ESP_OK : ESP_ERR_TIMEOUT; 
}



esp_err_t xUartPost(const uart_msg_t *msg){

    return xQueueSend(
        xUartQueue,
        msg,
        0) == pdPASS ? ESP_OK : ESP_ERR_TIMEOUT;

}

esp_err_t xUartReceive(uart_msg_t *msg,TickType_t timeout){

    return xQueueReceive(
        xUartQueue,
        msg,
        timeout) == pdPASS ? ESP_OK : ESP_ERR_TIMEOUT; 
}



esp_err_t xMqttPost(const mqtt_msg_t *msg){

    return xQueueSend(
        xMqttQueue,
        msg,
        0) == pdPASS ? ESP_OK : ESP_ERR_TIMEOUT;

}

esp_err_t xMqttReceive(mqtt_msg_t *msg,TickType_t timeout){

    return xQueueReceive(
        xMqttQueue,
        msg,
        timeout) == pdPASS ? ESP_OK : ESP_ERR_TIMEOUT; 
}