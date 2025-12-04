# CMakeLists.txt trong thư mục main/

idf_component_register(SRCS "main.c"
                      "mqtt_manager.c"
                      "relay_control.c"
                      "wifi_manager.c"
                      "ultrasonic_sensor.c"
                      "button_handler.c"
                      "timer_control.c"
                        "oled_display.c"
                      PRIV_REQUIRES spi_flash
                      PRIV_REQUIRES esp_driver_uart esp_driver_gpio
                      REQUIRES esp_timer driver
                      INCLUDE_DIRS 
                      "."
                      PRIV_REQUIRES ssd1306
                      REQUIRES
                      "esp_driver_gpio"
                      "esp_wifi"
                      "esp_event"
                      "nvs_flash"
                      "mqtt"
                      "esp_netif"
                  )