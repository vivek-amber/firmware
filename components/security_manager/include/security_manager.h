
#ifndef __SECURITY_MANAGER_H
#define __SECURITY_MANAGER_H

#include "esp_err.h"
#include <stdint.h>

esp_err_t xSecurityManagerInit( void );

esp_err_t xSecurityManagerSignNonce(const uint8_t *nonce,size_t nonceLen,uint8_t *signature,size_t signatureBufferSize,size_t *signatureLen );
esp_err_t xSecurityManagerVerifyClaim(const uint8_t *claim, size_t claimLen, uint8_t *signature, size_t signatureLen);

#endif