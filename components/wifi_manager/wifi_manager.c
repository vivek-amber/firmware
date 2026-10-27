

#include "wifi_manager.h"
#include "app_context.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "storage.h"
#include "queue_manager.h"

/* private variables */
static const char *TAG = "WIFI_MANAGER";
static uint8_t s_wifi_retry_count = 0;
#define WIFI_MAX_RETRIES 5

/* Private function decl */
static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
esp_err_t xWifimanagerStart(void);
esp_err_t xWifiManagerConnect(void);

esp_err_t xWifiManagerInit(void)
{

    /* Local vriable */
    app_context_t *ctx = xAppContextGet();

    /* Initialize network stack */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Event system initialization */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Creates default station interface */
    esp_netif_create_default_wifi_sta();

    /* Wifi driver init */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Config Wifi Mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    /* Register wifi - events to listen to */
    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));

    /* Register ip - events to listen to */
    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));

    /* Update state */
    ctx->wifi.state = WIFI_STATE_INITIALIZED;

    /* Load Credentials */

    return xWifimanagerStart();
}

esp_err_t xWifimanagerStart(void)
{

    /* Local variables */
    app_context_t *ctx = xAppContextGet();

    /* Load Credentials */
    esp_err_t err = xStorageLoadWifiCredentials();
    
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "wifi credentials found");
        /* Configure credentials*/
        if (strlen(ctx->wifi.credentials.ssid) == 0 || strlen(ctx->wifi.credentials.pass) == 0)
        {
            return ESP_ERR_INVALID_STATE;
        }

        wifi_config_t wifi_cfg = {0};
        strncpy((char *)wifi_cfg.sta.ssid, ctx->wifi.credentials.ssid, sizeof(wifi_cfg.sta.ssid) - 1);
        strncpy((char *)wifi_cfg.sta.password, ctx->wifi.credentials.pass, sizeof(wifi_cfg.sta.password) - 1);

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    }
    else
    {
        ESP_LOGI(TAG, "wifi credentials not found");
    }

    /* Start wifi driver */
    ESP_ERROR_CHECK(esp_wifi_start());

    return ESP_OK;
}

esp_err_t xWifiManagerConnect(void)
{
    app_context_t *ctx = xAppContextGet();

    esp_err_t err = esp_wifi_connect();

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_connect failed: %s",
                 esp_err_to_name(err));
        return err;
    }

    ctx->wifi.state = WIFI_STATE_CONNECTING;

    ESP_LOGI(TAG, "connecting to wifi");

    return ESP_OK;
}

esp_err_t xWifiManagerReset(void)
{
    app_context_t *ctx = xAppContextGet();

    wifi_config_t wifi_cfg = {0};

    strncpy((char *)wifi_cfg.sta.ssid, ctx->wifi.credentials.ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, ctx->wifi.credentials.pass, sizeof(wifi_cfg.sta.password) - 1);

    esp_wifi_disconnect();

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    return esp_wifi_connect();
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    app_context_t *ctx = xAppContextGet();

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {

        ESP_LOGI(TAG, "wifi started");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {

        s_wifi_retry_count = 0;
        ctx->wifi.connected = true;
        ctx->wifi.state = WIFI_STATE_CONNECTED;

        xEventPost(APP_EVENT_WIFI_CONNECTED);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {

        if (s_wifi_retry_count < WIFI_MAX_RETRIES)
        {
            s_wifi_retry_count++;
            esp_wifi_connect();
        }
        else{

            s_wifi_retry_count = 0;

            ctx->wifi.state = WIFI_STATE_DISCONNECTED;
            ctx->wifi.connected = false;
            xEventPost(APP_EVENT_WIFI_CONNECTION_FAILED);

        }

    }
}