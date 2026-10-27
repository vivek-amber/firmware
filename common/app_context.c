

#include "app_context.h"
#include "esp_mac.h"
#include "storage.h"

/* Private Variables */
static char deviceId[18];

/* Private functions */
static const char *getDeviceId(void);


static app_context_t g_app_ctx = {
    .app_state = APP_STATE_INIT,
    .ble = {
        .state = BLE_STATE_UNINITIALIZED,
        .advertising = false,
    },
    .wifi = {.connected = false, .state = WIFI_STATE_UNINITIALIZED},
};

esp_err_t xAppContextInit(void)
{
    g_app_ctx.deviceId = getDeviceId();

    return ESP_OK;
}

app_context_t *xAppContextGet(void)
{
    if (g_app_ctx.deviceId == NULL)
    {
        g_app_ctx.deviceId = getDeviceId();
    }

    return &g_app_ctx;
}

static const char *getDeviceId(void)
{

    uint8_t mac[6];

    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(
        deviceId,
        sizeof(deviceId),
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2],
        mac[3], mac[4], mac[5]);
    return deviceId;
}