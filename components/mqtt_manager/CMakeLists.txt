idf_component_register(
    SRCS "mqtt_topics.c" "mqtt_manager.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES mqtt common esp-tls queue_manager protobuf
)
