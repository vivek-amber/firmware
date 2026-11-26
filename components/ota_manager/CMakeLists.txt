idf_component_register(
    SRCS "ota_manager.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_timer esp_http_client esp_app_format mbedtls cjson common device_manager
)