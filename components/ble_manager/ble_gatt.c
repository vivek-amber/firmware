
#include "ble_gatt.h"
#include "queue_manager.h"
#include "app_context.h"

/* Private function declarations */
static void vGattProvisioningState(void);

static int wifi_cred_ssid_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int wifi_cred_pass_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int wifi_status_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

static int device_id_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int model_id_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables*/

static char modelId[] = "amber_ac_v1";
const ble_uuid128_t wifi_svc_uuid =
    BLE_UUID128_INIT(
        0x00, 0x00, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xa7,
        0xd4, 0x41,
        0x9b, 0xe2,
        0x00, 0x84, 0x0e, 0x55);

static const ble_uuid128_t wifi_ssid_char_uuid =
    BLE_UUID128_INIT(
        0x01, 0x00, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xa7,
        0xd4, 0x41,
        0x9b, 0xe2,
        0x00, 0x84, 0x0e, 0x55);

static const ble_uuid128_t wifi_pass_char_uuid =
    BLE_UUID128_INIT(
        0x02, 0x00, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xa7,
        0xd4, 0x41,
        0x9b, 0xe2,
        0x00, 0x84, 0x0e, 0x55);

static const ble_uuid128_t wifi_status_char_uuid =
    BLE_UUID128_INIT(
        0x03, 0x00, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xA7,
        0xD4, 0x41,
        0x9B, 0xE2,
        0x00, 0x84, 0x0E, 0x55);

static const ble_uuid128_t device_info_service_uuid =
    BLE_UUID128_INIT(
        0x00, 0x01, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xA7,
        0xD4, 0x41,
        0x9B, 0xE2,
        0x00, 0x84, 0x0E, 0x55);

static const ble_uuid128_t device_id_char_uuid =
    BLE_UUID128_INIT(
        0x01, 0x01, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xA7,
        0xD4, 0x41,
        0x9B, 0xE2,
        0x00, 0x84, 0x0E, 0x55);

static const ble_uuid128_t model_id_char_uuid =
    BLE_UUID128_INIT(
        0x02, 0x01, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xA7,
        0xD4, 0x41,
        0x9B, 0xE2,
        0x00, 0x84, 0x0E, 0x55);
static uint16_t ssid_chr_handle;
static uint16_t pass_chr_handle;
static uint16_t wifi_status_handle;

static uint8_t g_wifi_status = WIFI_STATUS_IDLE;

// To notify app about credentials ok
void vBleNotifyWifiStatus(uint16_t conn_handle, wifi_ble_status_t status)
{

    struct os_mbuf *om;

    om = ble_hs_mbuf_from_flat(
        &status,
        sizeof(status));

    if (om)
    {
        ble_gatts_notify_custom(
            conn_handle,
            wifi_status_handle,
            om);
    }
}

/* GATT Service Table*/
const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &wifi_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {.uuid = &wifi_ssid_char_uuid.u,
                 .access_cb = wifi_cred_ssid_cb,
                 .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
                 .val_handle = &ssid_chr_handle},
                {.uuid = &wifi_pass_char_uuid.u,
                 .access_cb = wifi_cred_pass_cb,
                 .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
                 .val_handle = &pass_chr_handle},
                {
                    .uuid = &wifi_status_char_uuid.u,
                    .access_cb = wifi_status_cb,
                    .flags = BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &wifi_status_handle,
                },
                {0},
            },
    },
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &device_info_service_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {
                 .uuid = &device_id_char_uuid.u,
                 .access_cb = device_id_cb,
                 .flags = BLE_GATT_CHR_F_READ,
             },
             {
                 .uuid = &model_id_char_uuid.u,
                 .access_cb = model_id_cb,
                 .flags = BLE_GATT_CHR_F_READ,
             },
             {0}}},
    {0},
};

esp_err_t xGattInit(void)
{

    /* 1. GATT Svc init*/
    ble_svc_gatt_init();

    /* 2. Update GATT service counter */
    int rc = ble_gatts_count_cfg(gatt_svr_svcs);

    ESP_LOGE("GATT", "count_cfg rc=%d", rc);

    if (rc != 0)
    {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);

    ESP_LOGE("GATT", "add_svcs rc=%d", rc);

    if (rc != 0)
    {
        return rc;
    }

    return ESP_OK;
}

static int wifi_cred_ssid_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{

    /* Local variable*/
    int err;
    app_context_t *ctx = xAppContextGet();

    /* Handle access events */
    switch (ctxt->op)
    {
    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_WRITE_CHR:

        size_t copy_len = MIN(ctxt->om->om_len, sizeof(ctx->wifi.credentials.ssid) - 1);

        err = os_mbuf_copydata(ctxt->om, 0, copy_len, ctx->wifi.credentials.ssid);

        if (err == ESP_OK)
        {
            ctx->wifi.credentials.ssid[copy_len] = '\0';
            ctx->wifi.credentials.ssid_valid = true;

            vGattProvisioningState();
        }

        return err;

    default:
        return BLE_ATT_ERR_UNLIKELY;
    }
}

static int wifi_cred_pass_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{

    /* Local variable*/
    int err;
    app_context_t *ctx = xAppContextGet();

    /* Handle access events */
    switch (ctxt->op)
    {
    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_WRITE_CHR:

        size_t copy_len = MIN(ctxt->om->om_len, sizeof(ctx->wifi.credentials.pass) - 1);

        err = os_mbuf_copydata(ctxt->om, 0, copy_len, ctx->wifi.credentials.pass);

        if (err == ESP_OK)
        {
            ctx->wifi.credentials.pass[copy_len] = '\0';
            ctx->wifi.credentials.pass_valid = true;

            vGattProvisioningState();
        }

        return err;

    default:
        return BLE_ATT_ERR_UNLIKELY;
    }
}

static int wifi_status_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{

    return 0;
}

/* Callback for deviceId read*/
static int device_id_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    app_context_t *ctx = xAppContextGet();

    os_mbuf_append(
        ctxt->om,
        ctx->deviceId,
        strlen(ctx->deviceId));

    return 0;
}

/* Callback for modelId read*/
static int model_id_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    os_mbuf_append(
        ctxt->om,
        modelId,
        strlen(modelId));

    return 0;
}

static void vGattProvisioningState(void)
{
    app_context_t *ctx = xAppContextGet();

    if (ctx->wifi.credentials.ssid_valid &&
        ctx->wifi.credentials.pass_valid)
    {

        app_context_t *ctx = xAppContextGet();
        ctx->wifi.credentials.pending_save = true;
        xEventPost(APP_EVENT_WIFI_PROVISIONING_COMPLETE);
    }
}