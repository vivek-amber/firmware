idf_component_register(
    SRCS               "uart_manager.c"
    INCLUDE_DIRS        "include"
    PRIV_REQUIRES       common esp_driver_uart queue_manager
)