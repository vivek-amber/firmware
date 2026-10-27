
#ifndef __LED_MANAGER_H
#define __LED_MANAGER_H

#include "app_types.h"

esp_err_t xLedManagerInit( void );
void vLedManagerSetDelay( uint32_t delayMs );

#endif