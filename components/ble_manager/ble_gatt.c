
#include "ble_gatt.h"
#include "queue_manager.h"
#include "app_context.h"
#include "security_manager.h"
#include "device_manager.h"

/* Private function declarations */
static void vGattProvisioningState(void);

static int wifi_cred_ssid_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int wifi_cred_pass_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int wifi_status_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static void provision_timer_cb(TimerHandle_t timer);

static int nonce_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int signature_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int claim_token_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int claim_signature_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int claim_status_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
void vGattNotifyClaimStatus(uint16_t conn_handle, uint8_t status);

static int device_id_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int model_id_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables*/

//static char modelId[] = "amber_ac_v1";
static TimerHandle_t provision_timer;
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

static const ble_uuid128_t provision_service_uuid =
    BLE_UUID128_INIT(
        0x00, 0x02, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xA7,
        0xD4, 0x41,
        0x9B, 0xE2,
        0x00, 0x84, 0x0E, 0x55);

static const ble_uuid128_t nonce_char_uuid =
    BLE_UUID128_INIT(
        0x01, 0x02, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xA7,
        0xD4, 0x41,
        0x9B, 0xE2,
        0x00, 0x84, 0x0E, 0x55);

static const ble_uuid128_t signature_char_uuid =
    BLE_UUID128_INIT(
        0x02, 0x02, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xA7,
        0xD4, 0x41,
        0x9B, 0xE2,
        0x00, 0x84, 0x0E, 0x55);

static const ble_uuid128_t claim_token_char_uuid =
    BLE_UUID128_INIT(
        0x03, 0x02, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xA7,
        0xD4, 0x41,
        0x9B, 0xE2,
        0x00, 0x84, 0x0E, 0x55);

static const ble_uuid128_t claim_signature_char_uuid =
    BLE_UUID128_INIT(
        0x04, 0x02, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xA7,
        0xD4, 0x41,
        0x9B, 0xE2,
        0x00, 0x84, 0x0E, 0x55);
static const ble_uuid128_t claim_status_char_uuid =
    BLE_UUID128_INIT(
        0x05, 0x02, 0x55, 0x66,
        0x44, 0x46,
        0x16, 0xA7,
        0xD4, 0x41,
        0x9B, 0xE2,
        0x00, 0x84, 0x0E, 0x55);

static uint16_t ssid_chr_handle;
static uint16_t pass_chr_handle;
static uint16_t wifi_status_handle;

static uint16_t nonce_handle;
static uint16_t signature_handle;
static uint16_t claim_token_handle;
static uint16_t claim_signature_handle;
static uint16_t claim_status_handle;

static char received_nonce[128] = {0};
static char claim_token[512];
static char claim_signature[256];

static uint8_t g_signature[128];
static size_t g_signature_len = 0;

static const char *TAG = "BLE_GATT";

// To notify app about credentials ok
void vBleNotifyWifiStatus(uint16_t conn_handle, wifi_ble_status_t status)
{

    struct os_mbuf *om;

    om = ble_hs_mbuf_from_flat(
        &status,
        sizeof(status));

    if (om)
    {
        int rc = ble_gatts_notify_custom(conn_handle, wifi_status_handle, om);

        ESP_LOGI(TAG, "wifi notify status=%d rc=%d", status, rc);
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
                 .flags = BLE_GATT_CHR_F_WRITE,
                 .val_handle = &ssid_chr_handle},
                {.uuid = &wifi_pass_char_uuid.u,
                 .access_cb = wifi_cred_pass_cb,
                 .flags = BLE_GATT_CHR_F_WRITE,
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
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &provision_service_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {
                    .uuid = &nonce_char_uuid.u,
                    .access_cb = nonce_cb,
                    .flags =
                        BLE_GATT_CHR_F_WRITE,
                    .val_handle = &nonce_handle,
                },

                {
                    .uuid = &signature_char_uuid.u,
                    .access_cb = signature_cb,
                    .flags =
                        BLE_GATT_CHR_F_READ,
                    .val_handle = &signature_handle,
                },

                {
                    .uuid = &claim_token_char_uuid.u,
                    .access_cb = claim_token_cb,
                    .flags =
                        BLE_GATT_CHR_F_WRITE,
                    .val_handle = &claim_token_handle,
                },
                {
                    .uuid = &claim_signature_char_uuid.u,
                    .access_cb = claim_signature_cb,
                    .flags =
                        BLE_GATT_CHR_F_WRITE,
                    .val_handle = &claim_signature_handle,
                },
                {
                    .uuid = &claim_status_char_uuid.u,
                    .access_cb = claim_status_cb,
                    .flags = BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &claim_status_handle,
                },

                {0},
            },
    },
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

    provision_timer = xTimerCreate("prov_timer", pdMS_TO_TICKS(300), pdFALSE, NULL, provision_timer_cb);

    return ESP_OK;
}

static int wifi_cred_ssid_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{

    /* Local variable*/
    int err;
    app_context_t *ctx = xAppContextGet();

    if (!ctx->security.claim_verified)
    {
        ESP_LOGE(TAG, "Claim not verified");
        return BLE_ATT_ERR_INSUFFICIENT_AUTHEN;
    }

    /* Handle access events */
    switch (ctxt->op)
    {
    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_WRITE_CHR:

        size_t copy_len = MIN(OS_MBUF_PKTLEN(ctxt->om), sizeof(ctx->wifi.credentials.ssid) - 1);

        err = os_mbuf_copydata(ctxt->om, 0, copy_len, ctx->wifi.credentials.ssid);

        if (err == ESP_OK)
        {
            ctx->wifi.credentials.ssid[copy_len] = '\0';
            ctx->wifi.credentials.ssid_received = true;

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

    if (!ctx->security.claim_verified)
    {
        ESP_LOGE(TAG, "Claim not verified");
        return BLE_ATT_ERR_INSUFFICIENT_AUTHEN;
    }

    /* Handle access events */
    switch (ctxt->op)
    {
    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_WRITE_CHR:

        size_t copy_len = MIN(OS_MBUF_PKTLEN(ctxt->om), sizeof(ctx->wifi.credentials.pass) - 1);

        err = os_mbuf_copydata(ctxt->om, 0, copy_len, ctx->wifi.credentials.pass);

        if (err == ESP_OK)
        {
            ctx->wifi.credentials.pass[copy_len] = '\0';
            ctx->wifi.credentials.pass_received = true;

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

static void provision_timer_cb(TimerHandle_t timer)
{
    ESP_LOGW(TAG, "Provision timer fired");
    xEventPost(APP_EVENT_WIFI_PROVISIONING_COMPLETE);
}

/* Callback for deviceId read*/
static int device_id_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{

    app_context_t *ctx = xAppContextGet();

    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (ctx == NULL || ctx->deviceId[0] == '\0')
    {
        ESP_LOGE(TAG, "Device ID is not available");
        return BLE_ATT_ERR_UNLIKELY;
    }

    int rc = os_mbuf_append(
        ctxt->om,
        ctx->deviceId,
        strlen(ctx->deviceId));

    if (rc != 0)
    {
        ESP_LOGE(TAG, "Failed to append device ID; rc=%d", rc);
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return 0;
}

/* Callback for modelId read*/
static int model_id_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{

    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }

    int rc = os_mbuf_append(
        ctxt->om,
        modelId,
        strlen(modelId));

    if (rc != 0)
    {
        ESP_LOGE(TAG, "Failed to append model ID; rc=%d", rc);
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return 0;
}

/* Callback for nonce write*/
static int nonce_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }

    size_t len = MIN(OS_MBUF_PKTLEN(ctxt->om), sizeof(received_nonce) - 1);
    os_mbuf_copydata(ctxt->om, 0, len, received_nonce);
    received_nonce[len] = '\0';

    esp_err_t rc = xSecurityManagerSignNonce(
        (const uint8_t *)received_nonce,
        len,
        g_signature,
        sizeof(g_signature),
        &g_signature_len);

    ESP_LOGI(TAG,
             "nonce len=%d sign rc=%d",
             len,
             rc);

    if (rc != ESP_OK)
    {
        ESP_LOGE(TAG, "Signature generation failed");
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

/* Callback for signature read*/
static int signature_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{

    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return os_mbuf_append(
        ctxt->om,
        g_signature,
        g_signature_len);
}

/* Callback for claim  data write*/
static int claim_token_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }

    size_t len =
        MIN(
            OS_MBUF_PKTLEN(ctxt->om),
            sizeof(claim_token) - 1);

    os_mbuf_copydata(
        ctxt->om,
        0,
        len,
        claim_token);

    claim_token[len] = '\0';

    ESP_LOGI(TAG, "claim token received");

    return 0;
}

/* Callback for claim signature write*/
static int claim_signature_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{

    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
        return BLE_ATT_ERR_UNLIKELY;
    }

    /* Local variable */
    app_context_t *ctx = xAppContextGet();

    size_t len = MIN(OS_MBUF_PKTLEN(ctxt->om), sizeof(claim_signature) - 1);
    os_mbuf_copydata(ctxt->om, 0, len, claim_signature);

    claim_signature[len] = '\0';

    esp_err_t rc = xSecurityManagerVerifyClaim((uint8_t *)claim_token, strlen(claim_token), (uint8_t *)claim_signature, strlen(claim_signature));

    if (rc != ESP_OK)
    {
        ctx->security.claim_verified = false;
        vGattNotifyClaimStatus(conn_handle, 0);

        return BLE_ATT_ERR_VALUE_NOT_ALLOWED;
    }

    ctx->security.claim_verified = true;
    vGattNotifyClaimStatus(conn_handle, 1);
    vGattProvisioningState();
    ESP_LOGI(TAG, "Claim verified");

    return 0;
}

/* Callback for claim status*/
static int claim_status_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    return 0;
}

void vGattNotifyClaimStatus(uint16_t conn_handle, uint8_t status)
{
    struct os_mbuf *om = ble_hs_mbuf_from_flat(&status, sizeof(status));

    if (om)
    {
        ble_gatts_notify_custom(conn_handle, claim_status_handle, om);
    }
}

static void vGattProvisioningState(void)
{
    app_context_t *ctx = xAppContextGet();

    if (ctx->wifi.credentials.ssid_received &&
        ctx->wifi.credentials.pass_received &&
        ctx->security.claim_verified &&
        !ctx->wifi.credentials.cred_save_pending)
    {

        app_context_t *ctx = xAppContextGet();
        ctx->wifi.credentials.cred_save_pending = true;
        xTimerReset(provision_timer, 0);
    }
}