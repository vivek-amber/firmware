/*
 * @@file: 
 * @@brief: 
 * @@author: Vivek
 * @@date: Do not edit
 */


#ifndef __APP_CONTEXT_H
#define __APP_CONTEXT_H

#include "app_types.h"

/* Tasks priorities */

#define PRIORITY_DEVICE_TASK        10U
#define PRIORITY_UART_TASK          5U
#define PRIORITY_MQTT_TASK          5U

typedef struct
{
    app_state_t app_state;
    ble_context_t ble;
    wifi_context_t wifi;
    mqtt_ctx_t mqtt;
    security_ctxt_t security;
    const char *deviceId;
    
} app_context_t;

esp_err_t xAppContextInit(void);
app_context_t *xAppContextGet( void );

#endif