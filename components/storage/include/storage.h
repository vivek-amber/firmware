
#ifndef __STORAGE_H
#define __STORAGE_H

#include "esp_err.h"
#include "esp_log.h"

/* Methods */
esp_err_t xStorageInit( void );
esp_err_t xStorageSet(const char *key, const char *value);
esp_err_t xStorageGet(const char *key, char *buffer, size_t len);
esp_err_t xStorageSaveWifiCredentials(void);
esp_err_t xStorageLoadWifiCredentials(void);
#endif