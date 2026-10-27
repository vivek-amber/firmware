
#ifndef __WIFI_MANAGER_H
#define __WIFI_MANAGER_H

#include "app_types.h"

/* Methods */
esp_err_t xWifiManagerInit(void);
esp_err_t xWifimanagerStart( void );
esp_err_t xWifiManagerConnect( void);
esp_err_t xWifiManagerReset(void);

#endif