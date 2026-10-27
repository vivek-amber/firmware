
#ifndef __BLE_COMMON_H
#define __BLE_COMMON_H

/* Includes */
/* STD APIs */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ESP APIs */
#include "esp_log.h"
#include "esp_err.h"


/* Defines */
#define DEVICE_NAME "BLE_DEVICE"

#define ADVERTISE_DURATION 120000 // 2 min

typedef enum {
    WIFI_STATUS_IDLE = 0,
    WIFI_STATUS_CONNECTING = 1,
    WIFI_STATUS_CONNECTED = 2,
    WIFI_STATUS_FAILED = 3,
    WIFI_STATUS_MQTT_CONNECTED = 4
} wifi_ble_status_t;


#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#endif