idf_component_register(
    SRCS "led_manager.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_gpio common
)