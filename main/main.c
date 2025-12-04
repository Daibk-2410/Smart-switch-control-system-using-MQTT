#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include "ssd1306.h"
#include "font8x8_basic.h"
#include "oled_display.h"

#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "relay_control.h"
#include "ultrasonic_sensor.h"
#include "button_handler.h"
#include "timer_control.h"

#define RELAY_PIN 26
#define ULTRASONIC_TRIG_PIN 19
#define ULTRASONIC_ECHO_PIN 18

static const char *TAG = "MAIN";

void ultrasonic_task(void *pvParameters)
{
    char distance_str[16];
    ESP_LOGI(TAG, "Ultrasonic task started.");

    while (1)
    {
        float distance = ultrasonic_get_distance_cm();

        // Log ra console để debug
        ESP_LOGI(TAG, "Distance: %.2f cm", distance);

        snprintf(distance_str, sizeof(distance_str), "%.2f", distance);
        mqtt_publish("home/relay1/distance", distance_str);

        // Đợi 2 giây trước khi đo lần tiếp theo
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void status_publish_task(void *pvParameters)
{
    char status_json[64];
    ESP_LOGI(TAG, "Status publish task started.");

    while (1)
    {
        int remaining_minutes = timer_get_remaining_seconds() / 60;

        snprintf(status_json, sizeof(status_json), "{\"timer_rem_min\":%d}", remaining_minutes);

        // Gửi trạng thái lên một topic riêng, ví dụ home/relay1/info
        mqtt_publish("home/relay1/info", status_json);

        // Đợi 30 giây mới gửi lại
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

void oled_update_task(void *pvParameters)
{
    ESP_LOGI(TAG, "OLED update task started.");
    while (1)
    {
        if (timer_is_active())
        {
            int remaining_seconds = timer_get_remaining_seconds();
            oled_update(OLED_STATE_ON_TIMER, remaining_seconds);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    oled_init();

    relay_init(RELAY_PIN);
    timer_init();
    button_init();
    ultrasonic_init(ULTRASONIC_TRIG_PIN, ULTRASONIC_ECHO_PIN);
    wifi_init_sta();

    vTaskDelay(pdMS_TO_TICKS(4000)); // Đợi WiFi ổn định
    mqtt_app_start();
    vTaskDelay(pdMS_TO_TICKS(2000)); // Đợi MQTT kết nối ổn định
    oled_update(OLED_STATE_OFF, 0);

    xTaskCreate(ultrasonic_task, "ultrasonic_task", 2048, NULL, 5, NULL);
    xTaskCreate(status_publish_task, "status_publish_task", 2048, NULL, 4, NULL);
    xTaskCreate(oled_update_task, "oled_update_task", 2048, NULL, 6, NULL);

    ESP_LOGI(TAG, "Initialization finished. Application tasks started.");
}
