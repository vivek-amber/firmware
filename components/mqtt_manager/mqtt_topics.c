
#include "mqtt_topics.h"
#include "pb_common.h"
#include "pb_encode.h"
#include "status.pb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

/* Private variables */
static const char *TAG = "MQTT_TPOICS";
static TimerHandle_t heartBeatTimer;
static esp_mqtt_client_handle_t mqttClient;

#define COMPANY_ID "amber03AABCA3456E1Z9"
#define HEART_BEAT_INTERVAL 5000 // 5 sec

/* Private Function declaration */
static void vMqttPublishHeartBeat(TimerHandle_t timer);

/* Susbcribes to topic and starts heart beat timer */
void vMqttSubscribeTopics(esp_mqtt_event_handle_t event)
{

    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

    app_context_t *ctx = xAppContextGet();

    char topic[128];
    snprintf(topic, sizeof(topic), "company/%s/device/%s/cmd", COMPANY_ID, ctx->deviceId);

    // Subscription
    esp_mqtt_client_subscribe(
        event->client,
        topic,
        1);

    /* Timer for periodic heart beats */
    mqttClient = event->client;
    heartBeatTimer = xTimerCreate(
        "heart_beat_timer",
        pdMS_TO_TICKS(HEART_BEAT_INTERVAL),
        pdTRUE,
        NULL,
        vMqttPublishHeartBeat);

    if (heartBeatTimer == NULL)
    {
        ESP_LOGE(TAG, "Timer create failed");
        return;
    }

    if (xTimerStart(heartBeatTimer, 0) != pdPASS)
    {
        ESP_LOGE(TAG, "Timer start failed");
        return;
    }

    ESP_LOGI(TAG, "Heartbeat timer started");
}

esp_err_t xMqttBuildStatusTopic(char *topic, size_t topicSize)
{
    app_context_t *ctx = xAppContextGet();

    int len = snprintf(
        topic,
        topicSize,
        "company/%s/device/%s/status",
        COMPANY_ID,
        ctx->deviceId);

    if (len < 0 || (size_t)len >= topicSize)
    {
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

size_t xMqttBuildStatusPayload(bool online,uint8_t *buffer,size_t bufferSize)
{
    device_DeviceStatus status = {
        .online = online};

    pb_ostream_t stream =
        pb_ostream_from_buffer(
            buffer,
            bufferSize);

    bool ok = pb_encode(
        &stream,
        device_DeviceStatus_fields,
        &status);

    if (!ok)
    {
        return 0;
    }

    return stream.bytes_written;
}

static void vMqttPublishHeartBeat(TimerHandle_t timer)
{
    if (mqttClient == NULL)
    {
        return;
    }

    char topic[128];

    ESP_ERROR_CHECK(
        xMqttBuildStatusTopic(
            topic,
            sizeof(topic)));

    uint8_t buffer[32];

    size_t payloadLen =
        xMqttBuildStatusPayload(
            true,
            buffer,
            sizeof(buffer));

    if (payloadLen == 0)
    {
        ESP_LOGE(TAG, "Encode failed");
        return;
    }

    int messageId =
        esp_mqtt_client_publish(
            mqttClient,
            topic,
            (const char *)buffer,
            payloadLen,
            1,
            0);

    ESP_LOGI(TAG,
             "Heartbeat sent msgId=%d",
             messageId);
}
