
#ifndef __OTA_MANAGER_H
#define __OTA_MANAGER_H

#pragma once

typedef struct {
    const char *model;
    const char *device_id;
} ota_manager_config_t;

esp_err_t xOtaManagerInit(void);
void ota_manager_on_wifi_connected(void);

#endif
