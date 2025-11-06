#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "stdio.h"

#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "relay_control.h"
#include "ultrasonic_sensor.h"


#define RELAY_PIN 26
#define ULTRASONIC_TRIG_PIN 19
#define ULTRASONIC_ECHO_PIN 18

static const char *TAG = "MAIN";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    relay_init(RELAY_PIN);
    ultrasonic_init(ULTRASONIC_TRIG_PIN, ULTRASONIC_ECHO_PIN);
    wifi_init_sta();

    vTaskDelay(pdMS_TO_TICKS(4000)); // Đợi WiFi ổn định
    mqtt_app_start();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    char distance_str[16];
    while(1){
        float distance = ultrasonic_get_distance_cm();
        ESP_LOGI(TAG, "Distance: %.2f cm", distance);

        // Chuyển đổi float thành string để gửi qua MQTT
        snprintf(distance_str, sizeof(distance_str), "%.2f", distance);

        // Gửi dữ liệu khoảng cách qua một topic MQTT mới
        mqtt_publish("home/relay1/distance", distance_str);
        
        // Đợi 2 giây trước khi đo lần tiếp theo
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
