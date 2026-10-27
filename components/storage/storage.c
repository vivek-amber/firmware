
#include "storage.h"
#include "nvs_flash.h"
#include "app_types.h"
#include "app_context.h"

static const char *TAG = "STORAGE";

#define NVS_NAMESPACE "storage"

esp_err_t xStorageInit(void)
{

    /* Local variable */
    esp_err_t err;

    err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to initialize storage stack");
    }

    return err;
}

esp_err_t xStorageSet(const char *key, const char *value)
{

    nvs_handle_t nvsHandler;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvsHandler);
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_str(nvsHandler, key, value);
    if (err == ESP_OK)
    {
        err = nvs_commit(nvsHandler);
    }

    nvs_close(nvsHandler);
    return err;
}

esp_err_t xStorageGet(const char *key, char *buffer, size_t len)
{

    nvs_handle_t nvsHandler;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvsHandler);
    if (err != ESP_OK)
    {
        return err;
    }

    size_t required_len = len;

    err = nvs_get_str(
        nvsHandler,
        key,
        buffer,
        &required_len);

    nvs_close(nvsHandler);

    return err;
}

esp_err_t xStorageDelete(const char *key)
{
    nvs_handle_t nvsHandler;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvsHandler);
    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_erase_key(nvsHandler, key);

    if (err == ESP_OK)
    {
        err = nvs_commit(nvsHandler);
    }

    nvs_close(nvsHandler);

    return err;
}

esp_err_t xStorageClearWifiCredentials(void)
{
    esp_err_t err;

    err = xStorageDelete("wifi_ssid");
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        return err;
    }

    err = xStorageDelete("wifi_pass");
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        return err;
    }

    app_context_t *ctx = xAppContextGet();

    memset(ctx->wifi.credentials.ssid, 0,
           sizeof(ctx->wifi.credentials.ssid));

    memset(ctx->wifi.credentials.pass, 0,
           sizeof(ctx->wifi.credentials.pass));

    ctx->wifi.credentials.pending_save = false;
    ctx->wifi.credentials.ssid_valid = false;
    ctx->wifi.credentials.pass_valid = false;

    ESP_LOGI(TAG, "wifi credentials cleared");

    return ESP_OK;
}

esp_err_t xStorageSaveWifiCredentials(void)
{
    app_context_t *ctx = xAppContextGet();

    esp_err_t err;

    err = xStorageSet("wifi_ssid", ctx->wifi.credentials.ssid);
    if (err != ESP_OK)
    {
        return err;
    }

    err = xStorageSet("wifi_pass", ctx->wifi.credentials.pass);
    if (err != ESP_OK)
    {
        return err;
    }

    ctx->wifi.credentials.pending_save = false;
    ctx->wifi.credentials.ssid_valid = false;
    ctx->wifi.credentials.pass_valid = false;

    ESP_LOGI(TAG, "wifi credentials saved, SSID: %s", ctx->wifi.credentials.ssid);

    return ESP_OK;
}

esp_err_t xStorageLoadWifiCredentials(void)
{
    app_context_t *ctx = xAppContextGet();

    esp_err_t err;

    err = xStorageGet("wifi_ssid",
                      ctx->wifi.credentials.ssid,
                      sizeof(ctx->wifi.credentials.ssid));

    if (err != ESP_OK)
    {
        return err;
    }

    err = xStorageGet(
        "wifi_pass",
        ctx->wifi.credentials.pass,
        sizeof(ctx->wifi.credentials.pass));

    if (err != ESP_OK)
    {
        return err;
    }

    ESP_LOGI(TAG, "wifi credentials loaded, SSID: %s", ctx->wifi.credentials.ssid);

    return ESP_OK;
}