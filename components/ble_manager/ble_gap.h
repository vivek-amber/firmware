/*
 * @@file:
 * @@brief:
 * @@author: Vivek
 * @@date:
 */
/*
 * @@file:
 * @@brief:
 * @@author: Vivek
 * @@date:
 */

#ifndef __BLE_GAP_H
#define __BLE_GAP_H

#include "ble_common.h"
#include "services/gap/ble_svc_gap.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"

/* Defines */
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200

esp_err_t xGapInit(void);
esp_err_t xGapAdvertiseStart(void);
esp_err_t xGapAdvertiseStop(void);
void ble_on_sync(void);

#endif