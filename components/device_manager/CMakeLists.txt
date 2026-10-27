idf_component_register(SRCS 
                        "device_manager.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES common ble_manager storage queue_manager wifi_manager uart_manager mqtt_manager devices led_manager
                    )
