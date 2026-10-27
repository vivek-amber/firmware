
#include "mqtt_manager.h"
#include "mqtt_client.h"
#include "esp_crt_bundle.h"
#include "queue_manager.h"
#include <stdbool.h>
#include "mqtt_topics.h"

/* Private variables */
static const char *TAG = "MQTT_MANAGER";
static const char *MQTT_URI = "mqtts://19b35c7aa9754ce0942e3dfd915f91b0.s1.eu.hivemq.cloud";
static const char *MQTT_USERNAME = "vivek";
static const char *MQTT_PASSWORD = "12345678";
static esp_mqtt_client_handle_t mqtt_client = NULL;

#define COMPANY_ID "amber03AABCA3456E1Z9"

/* Private func declarations*/
static void vMqttEventHandler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void vMqttConnectionHandler(esp_mqtt_event_id_t event_id, esp_mqtt_event_handle_t event);

/*********************** Main Body Starts **********************************/
esp_err_t xMqttManagerInit(void)
{

    app_context_t *ctx = xAppContextGet();

    size_t topic_size;
    char statusTopic[128];

    ESP_ERROR_CHECK(xMqttBuildStatusTopic(statusTopic,sizeof(statusTopic)));

    static uint8_t lwtBuffer[32];
    size_t lwtLen = xMqttBuildStatusPayload(false,lwtBuffer,sizeof(lwtBuffer));

    /* Configure and Init MQTT Client */
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = MQTT_URI,
                .port = 8883},
            .verification = {.crt_bundle_attach = esp_crt_bundle_attach}},
        .credentials = {.username = MQTT_USERNAME, .authentication = {.password = MQTT_PASSWORD}, .client_id = ctx->deviceId},
        .session = {.keepalive = 5, .last_will = {.topic = statusTopic, .msg = (const char *)lwtBuffer, .msg_len = lwtLen, .qos = 1, .retain = 1}}};
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    if (mqtt_client == NULL)
    {

        ESP_LOGI(TAG, "Mqtt client failed to initialize");
        return ESP_ERR_NOT_ALLOWED;
    }

    /* Register event handlers */
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, vMqttEventHandler, NULL));

    /* Start mqtt client */
    esp_mqtt_client_start(mqtt_client);

    return ESP_OK;
}

static void vMqttEventHandler(
    void *handler_arg,
    esp_event_base_t event_base,
    int32_t e_id,
    void *event_data)
{

    /* Type casted for better types */
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    esp_mqtt_event_id_t event_id = e_id;

    switch (event_id)
    {

    case MQTT_EVENT_CONNECTED:
    case MQTT_EVENT_DISCONNECTED:
        vMqttConnectionHandler(event_id, event);
        break;
    case MQTT_EVENT_DATA:

        mqtt_msg_t msg = {0};

        msg.len = MIN(event->data_len, sizeof(msg.data));
        memcpy(msg.data, event->data, msg.len);

        xMqttPost(&msg);
        xEventPost(APP_EVENT_MQTT_DATA);
        break;
    default:
        break;
    }
}

/* Handles connection callbacks */
static void vMqttConnectionHandler(esp_mqtt_event_id_t event_id, esp_mqtt_event_handle_t event)
{

    if (event_id == MQTT_EVENT_CONNECTED)
    {

        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        vMqttSubscribeTopics(event);
    }
    else
    {
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED, msg_id=%d", event->msg_id);
    }
}

bool bMqttmanagerSendData(const char *topic, const uint8_t *buff, const uint32_t len)
{

    if (topic == NULL || buff == NULL || len <= 0 || mqtt_client == NULL)
    {
        return false;
    }

    int messageId = esp_mqtt_client_publish(
        mqtt_client,
        topic,
        (const char *)buff,
        len,
        1,
        0);

    return messageId < 0 ? false : true;
}

esp_err_t xMqttManagerAckTopic(char *topic, size_t topic_size)
{

    /* Local variables */
    app_context_t *ctx = xAppContextGet();

    /* Topic creation */
    int topicLength = snprintf(
        topic,
        topic_size,
        "company/%s/device/%s/ack",
        COMPANY_ID,
        ctx->deviceId);

    if (topicLength < 0 ||
        (size_t)topicLength >= topic_size)
    {
        ESP_LOGE(TAG, "Ack topic is too long");
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}
esp_err_t xMqttManagerStatusTopic(char *topic, size_t topic_size)
{

    /* Local variables */
    app_context_t *ctx = xAppContextGet();

    /* Topic creation */
    int topicLength = snprintf(
        topic,
        topic_size,
        "company/%s/device/%s/status",
        COMPANY_ID,
        ctx->deviceId);

    if (topicLength < 0 ||
        (size_t)topicLength >= topic_size)
    {
        ESP_LOGE(TAG, "Ack topic is too long");
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

esp_err_t xMqttManagerStateTopic(char *topic, size_t topic_size)
{

    /* Local variables */
    app_context_t *ctx = xAppContextGet();

    /* Topic creation */
    int topicLength = snprintf(
        topic,
        topic_size,
        "company/%s/device/%s/state",
        COMPANY_ID,
        ctx->deviceId);

    if (topicLength < 0 ||
        (size_t)topicLength >= topic_size)
    {
        ESP_LOGE(TAG, "State topic is too long");
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}
