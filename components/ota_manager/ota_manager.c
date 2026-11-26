
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "ota_manager.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_app_desc.h"
#include "cJSON.h"
#include "device_manager.h"
#include "app_context.h"


#define OTA_VERSION_URL "https://yourserver.com/api/ota/version"
#define OTA_MAX_RESP_LEN 1024
#define OTA_CHECK_INTERVAL_US (60ULL * 1000000)  // 1 min for testing
//#define OTA_CHECK_INTERVAL_US (2ULL * 60 * 60 * 1000000)  // 2 Hrs

static const char *TAG = "ota_mgr";

typedef struct {
    bool update;
    char version[32];
    char url[256];
    int size;
    char sha256[65];
} ota_update_info_t;
static char s_resp_buf[OTA_MAX_RESP_LEN];

static TaskHandle_t       s_ota_task  = NULL;
static esp_timer_handle_t s_ota_timer = NULL;
static bool               s_started   = false;


static void ota_task(void *arg);
static void ota_timer_cb(void *arg);
static void ota_check_for_update(void);

esp_err_t xOtaManagerInit(void)
{
    xTaskCreate(ota_task, "ota_task", 8192, NULL, 4, &s_ota_task);

    const esp_timer_create_args_t timer_args = {
        .callback = ota_timer_cb,
        .name     = "ota_timer",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_ota_timer));

    ESP_LOGI(TAG, "OTA manager ready, waiting for Wi-Fi");
    return ESP_OK;
}

static char *ota_build_request_json(void)
{
    app_context_t *ctx = xAppContextGet();
    if (ctx == NULL) {
        ESP_LOGE(TAG, "App context not available");
        return NULL;
    }

    const esp_app_desc_t *app = esp_app_get_description();

    cJSON *root = cJSON_CreateObject();
    if(root == NULL)
    {
        return NULL;
    }

    cJSON_AddStringToObject(root, "model", modelId);
    cJSON_AddStringToObject(root, "current_version", app->version);
    cJSON_AddStringToObject(root, "device_id", ctx->deviceId);   

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static esp_err_t ota_https_post(const char *url, const char *body, char *resp, size_t resp_len)
{
    esp_err_t err = ESP_OK;
    int body_len = strlen(body);
    int total = 0;
    int n = 0;

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "HTTP client init failed");
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");

    /* 1. Connect (DNS + TLS handshake + certificate check happen here) */
    err = esp_http_client_open(client, body_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Connection failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    /* 2. Send the JSON body */
    if (esp_http_client_write(client, body, body_len) != body_len) {
        ESP_LOGE(TAG, "Failed to send request body");
        err = ESP_FAIL;
        goto cleanup;
    }

     /* 3. Read status line + headers */
    esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
        ESP_LOGE(TAG, "Server returned HTTP %d", status);
        err = ESP_FAIL;
        goto cleanup;
    }

    /* 4. Read the body into resp, leaving room for '\0' */
    while ((n = esp_http_client_read(client, resp + total,
                                     resp_len - 1 - total)) > 0) {
        total += n;
        if (total >= (int)resp_len - 1) {
            ESP_LOGE(TAG, "Response larger than %d bytes", (int)resp_len - 1);
            err = ESP_ERR_INVALID_SIZE;
            goto cleanup;
        }
    }
    if (n < 0) {
        ESP_LOGE(TAG, "Error reading response");
        err = ESP_FAIL;
        goto cleanup;
    }
    resp[total] = '\0';

cleanup:
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return err;
}

static esp_err_t ota_parse_response(const char *json, ota_update_info_t *info)
{
    esp_err_t err = ESP_FAIL;
    const cJSON *item;

    memset(info, 0, sizeof(*info));

    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        ESP_LOGE(TAG, "Response is not valid JSON");
        return ESP_FAIL;
    }

    item = cJSON_GetObjectItem(root, "update");
    if (!cJSON_IsBool(item)) {
        ESP_LOGE(TAG, "Missing or invalid 'update' field");
        goto out;
    }
    info->update = cJSON_IsTrue(item);

    if (!info->update) {
        err = ESP_OK;     
        goto out;
    }

    item = cJSON_GetObjectItem(root, "version");
    if (!cJSON_IsString(item) || strlen(item->valuestring) >= sizeof(info->version)) {
        ESP_LOGE(TAG, "Missing or invalid 'version'");
        goto out;
    }
    strlcpy(info->version, item->valuestring, sizeof(info->version));

    item = cJSON_GetObjectItem(root, "url");
    if (!cJSON_IsString(item) || strlen(item->valuestring) >= sizeof(info->url)) {
        ESP_LOGE(TAG, "Missing or invalid 'url'");
        goto out;
    }
    strlcpy(info->url, item->valuestring, sizeof(info->url));

    item = cJSON_GetObjectItem(root, "size");
    if (!cJSON_IsNumber(item) || item->valueint <= 0) {
        ESP_LOGE(TAG, "Missing or invalid 'size'");
        goto out;
    }
    info->size = item->valueint;

    item = cJSON_GetObjectItem(root, "sha256");
    if (!cJSON_IsString(item) || strlen(item->valuestring) != 64) {
        ESP_LOGE(TAG, "Missing or invalid 'sha256' (must be 64 hex chars)");
        goto out;
    }
    strlcpy(info->sha256, item->valuestring, sizeof(info->sha256));

    err = ESP_OK;

out:
    cJSON_Delete(root);
    return err;
}
static void ota_check_for_update(void)
{
    ESP_LOGI(TAG, "Checking for update...");
    char *body = ota_build_request_json();
    if (body == NULL) {
        ESP_LOGE(TAG, "Failed to build request");
        return;
    }
    ESP_LOGI(TAG, "Request: %s", body);

    esp_err_t err = ota_https_post(OTA_VERSION_URL, body,
                                  s_resp_buf, sizeof(s_resp_buf));
    cJSON_free(body);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Version check failed, will retry at next interval");
        return;
    }
    ESP_LOGI(TAG, "Response: %s", s_resp_buf);

    ota_update_info_t info;
    if (ota_parse_response(s_resp_buf, &info) != ESP_OK) {
        return;
    }

    if (!info.update) {
        ESP_LOGI(TAG, "Firmware is up to date");
        return;
    }

    ESP_LOGI(TAG, "Update available: %s (%d bytes)", info.version, info.size);
    ESP_LOGI(TAG, "URL: %s", info.url);
}

static void ota_timer_cb(void *arg)
{
    xTaskNotifyGive(s_ota_task);
}

static void ota_task(void *arg)
{
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ota_check_for_update();
    }
}

void ota_manager_on_wifi_connected(void)
{
    if (s_ota_task == NULL || s_started) {
        return;   /* not initialised yet, or already running */
    }
    s_started = true;

    ESP_LOGI(TAG, "Wi-Fi up: first check now, then every 2 h");
    xTaskNotifyGive(s_ota_task);     
    ESP_ERROR_CHECK(
        esp_timer_start_periodic(s_ota_timer, OTA_CHECK_INTERVAL_US)); 
}

