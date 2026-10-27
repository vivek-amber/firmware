idf_component_register(SRCS 
                        "wifi_manager.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES
                        common
                        esp_netif
                        esp_event
                        esp_wifi
                        storage
                        queue_manager
                    )
