
#ifndef __BLE_GATT_H
#define __BLE_GATT_H

#include "ble_common.h"
#include "services/gatt/ble_svc_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"



esp_err_t xGattInit( void );

extern const ble_uuid128_t wifi_svc_uuid;

#endif
