# CMakeLists.txt trong thư mục main/

idf_component_register(SRCS "main.c"
                      "mqtt_manager.c"
                      "relay_control.c"
                      "wifi_manager.c"
                INCLUDE_DIRS ".")