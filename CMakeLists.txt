idf_component_register(SRC_DIRS "source"  "source/report" "SH2"
                    INCLUDE_DIRS "." "include" "include/report" "include/callback" "SH2"
                    REQUIRES driver esp_driver_gpio esp_driver_spi esp_timer cmock)