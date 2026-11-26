/*
 * @@file:
 * @@brief:
 * @@author: Vivek
 * @@date:
 */

#include "security_manager.h"
#include "psa/crypto.h"
#include "mbedtls/base64.h"
#include "app_context.h"
#include "cJSON.h"
#include "esp_log.h"

static const char *TAG = "SECURITY";
static psa_key_id_t sPrivateKeyId = 0;
static psa_key_id_t sServerPublicKeyId = 0;

static void vGenerateAndPrintKeyPair(void);

static const uint8_t PRIVATE_KEY[32] = {
    0x51, 0x9b, 0x42, 0x3d, 0x71, 0x5f, 0x8b, 0x58,
    0x1f, 0x4e, 0x07, 0x54, 0x96, 0xd8, 0xe0, 0x4a,
    0xe2, 0xb8, 0x7c, 0x30, 0x94, 0x76, 0x5a, 0xc3,
    0x7c, 0xb1, 0x7a, 0x2d, 0x54, 0x11, 0x8e, 0x99};

static const uint8_t SERVER_PUBLIC_KEY[65] = {
0x04, 0x9B, 0x2C, 0xEB, 0x7F, 0x23, 0x5B, 0xFF, 
0x76, 0xE9, 0x37, 0x2E, 0x61, 0x16, 0x30, 0x73, 
0xF4, 0x7E, 0xE1, 0x35, 0x27, 0xA5, 0x0C, 0x16, 
0xCC, 0x92, 0x22, 0x48, 0x7E, 0x37, 0x8C, 0x53, 
0xC6, 0x0F, 0xD9, 0x69, 0x1F, 0x30, 0xBA, 0xC0, 
0xDB, 0x14, 0x86, 0xFE, 0xF0, 0x1F, 0x5C, 0xF6, 
0xF3, 0x9F, 0x3D, 0x00, 0x29, 0x5A, 0x8F, 0xA2, 
0xDD, 0x92, 0xC1, 0xA1, 0x17, 0x4F, 0xE4, 0x6F, 
0xA2
};

esp_err_t xSecurityManagerInit(void)
{

    psa_status_t status;

    /* Initializes crypto framework */
    status = psa_crypto_init();

    if (status != PSA_SUCCESS)
    {
        ESP_LOGE(TAG, "psa_crypto_init failed");
    }

    // Private key load
    /* This holds all the props of key */
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);

    /* Assigns key id to use locally instead of using orig key */
    status = psa_import_key(&attributes, PRIVATE_KEY, sizeof(PRIVATE_KEY), &sPrivateKeyId);

    if (status != PSA_SUCCESS)
    {
        ESP_LOGE(TAG,
                 "Private key import failed %ld",
                 (long)status);
        return ESP_FAIL;
    }

    // Server public key load
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
    psa_set_key_bits(&attributes, 256);

    status = psa_import_key(&attributes, SERVER_PUBLIC_KEY, sizeof(SERVER_PUBLIC_KEY), &sServerPublicKeyId);

    /* Cleanup for mem used for key props */
    psa_reset_key_attributes(&attributes);

    if (status != PSA_SUCCESS)
    {
        ESP_LOGE(TAG, "psa_import_key failed %ld", (long)status);
        return ESP_FAIL;
    }

    //  vGenerateAndPrintKeyPair();

    return ESP_OK;
}

esp_err_t xSecurityManagerSignNonce(const uint8_t *nonce, size_t nonceLen, uint8_t *signature, size_t signatureBufferSize, size_t *signatureLen)
{

    psa_status_t status;
    uint8_t nonce_bin[32];
    size_t decodedLen = 0;
    ESP_LOGI(TAG, "nonce len=%u", (unsigned)nonceLen);

    int rc = mbedtls_base64_decode(nonce_bin, sizeof(nonce_bin), &decodedLen, nonce, nonceLen);

    if (rc != 0)
    {
        ESP_LOGE(TAG, "base64 decode failed %d", rc);
        return ESP_FAIL;
    }

    status = psa_sign_message(
        sPrivateKeyId,
        PSA_ALG_ECDSA(PSA_ALG_SHA_256),
        nonce_bin,
        decodedLen,
        signature,
        signatureBufferSize,
        signatureLen);

    ESP_LOGI(TAG, "SIGNATURE LEN=%u", (unsigned)*signatureLen);

    if (status != PSA_SUCCESS)
    {

        ESP_LOGE(TAG, "psa_sign_message failed %ld", (long)status);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t xSecurityManagerVerifyClaim(const uint8_t *claim, size_t claimLen, uint8_t *signature, size_t signatureLen)
{

    /* Local Variables */
    app_context_t *ctx = xAppContextGet();
    psa_status_t status;
    int rc = 0;
    uint8_t sigBin[128];
    size_t sigBinLen;

    rc = mbedtls_base64_decode(sigBin, sizeof(sigBin), &sigBinLen, signature, signatureLen);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "base64 decode failed %d", rc);
        return ESP_FAIL;
    }

    status = psa_verify_message(sServerPublicKeyId, PSA_ALG_ECDSA(PSA_ALG_SHA_256), claim, claimLen, sigBin, sigBinLen);

    if (status != PSA_SUCCESS)
    {
        ESP_LOGE(TAG, "Claim verification failed");
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse((char *)claim);

    if (root == NULL)
    {
        ESP_LOGE(TAG, "Invalid claim JSON");
        return ESP_FAIL;
    }

    cJSON *deviceId = cJSON_GetObjectItem(root, "deviceId");
    if (!cJSON_IsString(deviceId))
    {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    if (strcmp(deviceId->valuestring, ctx->deviceId) != 0)
    {
        ESP_LOGE(TAG, "Device ID mismatch");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON *timestamp = cJSON_GetObjectItem(root, "timeStamp");

    if (!cJSON_IsNumber(timestamp))
    {
        cJSON_Delete(root);
        return ESP_FAIL;
    }


    cJSON_Delete(root);
    ESP_LOGI(TAG, "Claim verification successful");
    return ESP_OK;
}

static void vGenerateAndPrintKeyPair(void)
{
    psa_key_id_t keyId;
    psa_status_t status;

    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

    psa_set_key_usage_flags(&attr,
                            PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_EXPORT);

    psa_set_key_algorithm(&attr,
                          PSA_ALG_ECDSA(PSA_ALG_SHA_256));

    psa_set_key_type(&attr,
                     PSA_KEY_TYPE_ECC_KEY_PAIR(
                         PSA_ECC_FAMILY_SECP_R1));

    psa_set_key_bits(&attr, 256);

    status = psa_generate_key(&attr, &keyId);

    if (status != PSA_SUCCESS)
    {
        ESP_LOGE(TAG,
                 "key generation failed %ld",
                 (long)status);
        return;
    }

    uint8_t privateKey[64];
    size_t privateKeyLen = 0;

    status = psa_export_key(keyId,
                            privateKey,
                            sizeof(privateKey),
                            &privateKeyLen);

    if (status != PSA_SUCCESS)
    {
        ESP_LOGE(TAG,
                 "private export failed %ld",
                 (long)status);
        return;
    }

    uint8_t publicKey[100];
    size_t publicKeyLen = 0;

    status = psa_export_public_key(keyId,
                                   publicKey,
                                   sizeof(publicKey),
                                   &publicKeyLen);

    if (status != PSA_SUCCESS)
    {
        ESP_LOGE(TAG,
                 "public export failed %ld",
                 (long)status);
        return;
    }

    printf("\n\nPRIVATE KEY [%u]\n",
           (unsigned)privateKeyLen);

    printf("static const uint8_t PRIVATE_KEY[%u] = {\n",
           (unsigned)privateKeyLen);

    for (size_t i = 0; i < privateKeyLen; i++)
    {
        printf("0x%02X",
               privateKey[i]);

        if (i != privateKeyLen - 1)
        {
            printf(", ");
        }

        if ((i + 1) % 8 == 0)
        {
            printf("\n");
        }
    }

    printf("\n};\n\n");

    printf("PUBLIC KEY [%u]\n",
           (unsigned)publicKeyLen);

    printf("static const uint8_t SERVER_PUBLIC_KEY[%u] = {\n",
           (unsigned)publicKeyLen);

    for (size_t i = 0; i < publicKeyLen; i++)
    {
        printf("0x%02X",
               publicKey[i]);

        if (i != publicKeyLen - 1)
        {
            printf(", ");
        }

        if ((i + 1) % 8 == 0)
        {
            printf("\n");
        }
    }

    printf("\n};\n\n");

    psa_destroy_key(keyId);
}