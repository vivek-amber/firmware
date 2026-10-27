
#ifndef __UART_MANAGER_H
#define __UART_MANAGER_H

#include "app_types.h"
#include "app_context.h"

esp_err_t xUartManagerInit( void );
bool bSendDataToDevice( const uint8_t * buff, const uint8_t len );

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#endif