
#include "ble_manager.h"
#include "ble_gatt.h"
#include "ble_gap.h"
#include "nimble/nimble_port_freertos.h"
#include "nimble/nimble_port.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "app_context.h"

/* Private function declarations */
static void nimble_host_task(void *param);

/* Private variables */
static const char *TAG = "BLE_MANAGER";

static void nimble_host_task(void *param){
    
    /* Task entry log */
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();

    /* Clean up at exit */
    nimble_port_freertos_deinit();
}

esp_err_t xBleManagerInit( void ){

    /* Local variable */
    esp_err_t err;
    app_context_t *ctx = xAppContextGet();

    /* NimBLE stack initialization */
    err = nimble_port_init();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "failed to initialize nimble stack");
        return err;
    }

    ble_hs_cfg.sync_cb = ble_on_sync;

    /* Security */
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;

    /* GAP service initialization */
    err = xGapInit();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "failed to initialize GAP service");
        nimble_port_deinit();
        return err;
    }

    /* GATT server initialization */
    err = xGattInit();
    if(err != ESP_OK){
        ESP_LOGE(TAG, "failed to initialize GATT service");
        nimble_port_deinit();
        return err;
    }

    /* Start NimBLE host task thread and return */
    nimble_port_freertos_init(nimble_host_task);
    ctx->ble.state = BLE_STATE_INITIALIZED;

    return ESP_OK;
}

esp_err_t xBleManagerStartAdvertising( void ){

    return xGapAdvertiseStart();
}

esp_err_t xBleManagerStopAdvertising( void ){
    return xGapAdvertiseStop();
}