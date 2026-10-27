
#include "ble_gap.h"
#include "app_context.h"
#include "ble_gatt.h"


/* Private variables */
static const char *TAG = "BLE_GAP";
static uint8_t own_addr_type;

/* Private function*/
static void vGapSetOwnAddrType(uint8_t addr_type);
static esp_err_t gap_event_handler(struct ble_gap_event *event, void *arg);
inline static void format_addr(char *addr_str, uint8_t addr[])
{

    sprintf(
        addr_str,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}

esp_err_t xGapInit(void)
{
    /* Local variables*/
    esp_err_t err;

    /* Init Gap Service */
    ble_svc_gap_init();

    /* Set Device Name */
    ESP_ERROR_CHECK(ble_svc_gap_device_name_set(DEVICE_NAME));

    /* Set Device Appearance */
    ESP_ERROR_CHECK(ble_svc_gap_device_appearance_set(BLE_GAP_APPEARANCE_GENERIC_TAG));

    return ESP_OK;
}

esp_err_t xGapAdvertiseStart(void)
{

    /* Local variables */
    esp_err_t err;
    const char *name;
    app_context_t *ctx = xAppContextGet();

    struct ble_hs_adv_fields adv_fields = {0};
    struct ble_hs_adv_fields rsp_fields = {0};
    struct ble_gap_adv_params adv_params = {0};

    /* Advertising Data */
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    
    /* Set device name */
    name = ble_svc_gap_device_name();
    if(name == NULL){
        name = DEVICE_NAME;
    };

    adv_fields.name = (uint8_t *) name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;

    /* Set device tx power */
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;

    /* Set device appearance */
    adv_fields.appearance = BLE_GAP_APPEARANCE_GENERIC_TAG;
    adv_fields.appearance_is_present = 1;

    /* Set advertisement fields */
    err = ble_gap_adv_set_fields(&adv_fields);
    if(err != ESP_OK){
        ESP_LOGE(TAG, " failed to set advertising data");
        return err;
    }

    /* Advertise provisioning service UUID in scan response */
    rsp_fields.uuids128 = (ble_uuid128_t *)&wifi_svc_uuid;
    rsp_fields.num_uuids128 = 1;
    rsp_fields.uuids128_is_complete = 1;

    /* Set scan response fields */
    err = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, " failed to set scan response data");
        return err;
    }

    /* Set connectable and general discoverable mode */
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    /* Start advertising */
    err = ble_gap_adv_start(own_addr_type,NULL,ADVERTISE_DURATION,&adv_params,gap_event_handler,NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, " failed to start advertising");
        return err;
    }

    ctx ->ble.state = BLE_STATE_ADVERTISING;
    ctx ->ble.advertising = true;

    ESP_LOGI(TAG, " advertising started!");
    return ESP_OK;
}

void vGapSetOwnAddrType(uint8_t addr_type)
{
    own_addr_type = addr_type;
}

esp_err_t xGapAdvertiseStop( void ){

    return ble_gap_adv_stop();
}

static int gap_event_handler(struct ble_gap_event *event, void *arg){

    app_context_t *ctx = xAppContextGet();

    /* Handle different GAP event */
    switch (event ->type)
    {
        /* Connect event */
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "connection %s",
                event->connect.status == 0 ? "established" : "failed"
            );
            
            if(event->connect.status == 0)
            {
                ctx->ble.state = BLE_STATE_CONNECTED;
                ctx->ble.advertising = false;

            }
            return 0;
    
        case BLE_GAP_EVENT_ENC_CHANGE:
            ESP_LOGI(TAG,"Encryption %s",event->enc_change.status == 0
            ? "enabled"
            : "failed"
            );
        return 0;
        /* Disconnect event */
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "disconnected from peer; reason=%d", event->disconnect.reason);
            
            /* Update state */
            ctx->ble.state = BLE_STATE_DISCONNECTED;
            ctx->ble.conn_handle = BLE_HS_CONN_HANDLE_NONE;
            return 0;

        /* Advertise timeout*/
        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "advertise complete; reason=%d",event->adv_complete.reason);
            
            /* Update state */
            ctx->ble.state = BLE_STATE_INITIALIZED;
            ctx->ble.advertising = false;
            return 0;
    }

    return 0;

}

void ble_on_sync(void)
{
    int rc;
    uint8_t addr_type;

    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    rc = ble_hs_id_infer_auto(0,&addr_type);
    assert(rc == 0);

    vGapSetOwnAddrType(addr_type);
}