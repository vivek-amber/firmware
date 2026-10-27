
#ifndef __APP_TYPES_H
#define __APP_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>

typedef enum
{
    APP_STATE_INIT,
    APP_STATE_RUNNING,
    APP_STATE_ERROR
} app_state_t;

typedef enum
{
    APP_EVENT_NONE = 0,

    APP_EVENT_WIFI_PROVISIONING_START,
    APP_EVENT_WIFI_PROVISIONING_COMPLETE,
    APP_EVENT_WIFI_CONNECTION_FAILED,

    APP_EVENT_WIFI_CONNECT,
    APP_EVENT_WIFI_CONNECTED,
    APP_EVENT_WIFI_DISCONNECTED,

    APP_EVENT_BLE_CONNECTED,
    APP_EVENT_BLE_DISCONNECTED,

    APP_EVENT_MQTT_CONNECTED,
    APP_EVENT_MQTT_DISCONNECTED,

    APP_EVENT_UART_DATA,
    APP_EVENT_MQTT_DATA

} app_event_t;

/* BLE Types */
typedef enum
{
    BLE_STATE_UNINITIALIZED = 0,
    BLE_STATE_INITIALIZED,
    BLE_STATE_ADVERTISING,
    BLE_STATE_CONNECTED,
    BLE_STATE_DISCONNECTED,
    BLE_STATE_ERROR

} ble_state_t;

typedef struct
{

    ble_state_t state;
    bool advertising;

    uint16_t conn_handle;

} ble_context_t;

/* Wifi Types */
typedef enum
{
    WIFI_STATE_UNINITIALIZED = 0,
    WIFI_STATE_INITIALIZED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_DISCONNECTING,
    WIFI_STATE_ERROR
} wifi_state_t;

typedef struct{
    char ssid[33];
    char pass[65];

    bool ssid_valid;
    bool pass_valid;

    bool pending_save;
} wifi_credentials_t;

typedef struct
{
    wifi_state_t state;
    bool connected;

    wifi_credentials_t credentials;

} wifi_context_t;

typedef struct
{
    uint16_t len;
    uint8_t data[512];
} uart_msg_t;

typedef struct
{
    uint8_t data[256];
    uint16_t len;
} mqtt_msg_t;


#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#endif