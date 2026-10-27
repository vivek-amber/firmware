idf_component_register(SRCS 
                        "ble_manager.c"
                        "ble_gap.c"
                        "ble_gatt.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES
                        bt 
                        common
                        queue_manager
                    )
