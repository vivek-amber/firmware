

#ifndef __BLE_MANAGER_H
#define __BLE_MANAGER_H

#include "ble_common.h"

/* Methods */
esp_err_t xBleManagerInit( void );
esp_err_t xBleManagerStartAdvertising( void );
esp_err_t xBleManagerStopAdvertising( void );

extern  void vBleNotifyWifiStatus(uint16_t conn_handle,wifi_ble_status_t status);

#endif