
#include "device_manager.h"
#include "app_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "storage.h"
#include "ble_manager.h"
#include "wifi_manager.h"
#include "queue_manager.h"
#include "uart_manager.h"
#include "mqtt_manager.h"
#include "ac_device.h"
#include "storage.h"
#include "led_manager.h"
#include "security_manager.h"

/* Private variables*/
static const char *TAG = "DEVICE_MANAGER";

/* Private Task */
static void vDeviceTask(void *arg);

void vDeviceManagerInit(void)
{

    /* Init managers */
    xQueueManagerInit();
    xStorageInit();
    xSecurityManagerInit();
    xAppContextInit();
    xBleManagerInit();
    xWifiManagerInit();
    xUartManagerInit();
    vDeviceInit();
    xLedManagerInit();

    /* Create device orchestration task */
    xTaskCreate(vDeviceTask, "device_task", 4096, NULL, 5, NULL);
}

static void vDeviceTask(void *arg)
{

    /* Local Variable */
    app_event_t event;
    app_context_t *ctx = xAppContextGet();

    while (1)
    {

        if (xEventReceive(&event, portMAX_DELAY) == ESP_OK)
        {

            switch (event)
            {
            case APP_EVENT_UART_DATA:
            {

                uart_msg_t msg;
                if (xUartReceive(&msg, 0) == ESP_OK)
                {
                    vAcDeviceProcessUartFrame(&msg);
                }
                break;
            }
            case APP_EVENT_MQTT_DATA:
            {

                mqtt_msg_t msg;

                if (xMqttReceive(&msg, portMAX_DELAY) == ESP_OK)
                {
                    vAcDeviceProcessCloudCommand(&msg);
                }
                break;
            }

            case APP_EVENT_WIFI_PROVISIONING_START:
                // 1. Start Advertisement
                xBleManagerStartAdvertising();
                break;

            case APP_EVENT_WIFI_PROVISIONING_COMPLETE:

                // 1. Stop MQTT client if it was already initialized 
                if (ctx->mqtt.initialized)
                {
                    xMqttManagerStop(); // MQTT stop function 
                    ctx->mqtt.initialized = false;
                }

                // Notify's app
                vBleNotifyWifiStatus(ctx->ble.conn_handle, WIFI_STATUS_CONNECTING);
                xBleManagerStopAdvertising();
                xWifiManagerReset();
                break;

            case APP_EVENT_WIFI_CONNECTED:

                ESP_LOGI(TAG, "wifi connected");

                // Notify's app
                vBleNotifyWifiStatus(ctx->ble.conn_handle, WIFI_STATUS_CONNECTED);

                if (ctx->wifi.credentials.pending_save)
                {
                    esp_err_t err = xStorageSaveWifiCredentials();

                    if (err != ESP_OK)
                    {
                        ESP_LOGE(TAG, "Failed to save credentials");
                    }
                }

                if (!ctx->mqtt.initialized)
                {
                    if (xMqttManagerInit() == ESP_OK)
                    {
                        ctx->mqtt.initialized = true;
                    }
                    else
                    {
                        ESP_LOGE(TAG, "MQTT init failed");
                    }
                }
                break;

            case APP_EVENT_WIFI_CONNECTION_FAILED:

                // Notify's app
                if (ctx->wifi.credentials.pending_save)
                {
                    vBleNotifyWifiStatus(ctx->ble.conn_handle, WIFI_STATUS_FAILED);
                    ctx->wifi.credentials.pending_save = false;
                }
                break;

            default:
                break;
            }
        }
    }
}