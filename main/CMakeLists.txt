idf_component_register(SRCS "main.c" "wifi.c" "web_site.c" "temperature.c" "heater.c"
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)
