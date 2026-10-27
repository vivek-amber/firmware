idf_component_register(
    SRCS "ac_device.c" "ac_device_utils.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES common protobuf uart_manager mqtt_manager
)