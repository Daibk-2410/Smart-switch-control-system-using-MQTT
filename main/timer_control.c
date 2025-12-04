#include "timer_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "relay_control.h"
#include "esp_log.h"
#include "mqtt_manager.h"
#include "oled_display.h"
#include <stdbool.h>

static const char *TAG = "TIMER_CTRL";
static TimerHandle_t relay_timer_handle = NULL;
static TickType_t total_ticks_on_start = 0;
static TickType_t start_tick = 0;

// Hàm callback này sẽ được gọi khi timer kết thúc
void relay_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "Timer expired. Turning relay OFF.");
    relay_set(0);                              // Tắt relay
    mqtt_publish("home/relay1/status", "OFF"); // Cập nhật trạng thái qua MQTT
    total_ticks_on_start = 0;
}

void timer_init()
{
    // Tạo timer một lần duy nhất, nhưng chưa khởi động
    relay_timer_handle = xTimerCreate(
        "RelayTimer",        // Tên timer
        pdMS_TO_TICKS(1000), // Period (không quan trọng vì là one-shot)
        pdFALSE,             // Không tự động lặp lại (one-shot timer)
        (void *)0,           // Timer ID
        relay_timer_callback // Hàm callback
    );

    if (relay_timer_handle == NULL)
    {
        ESP_LOGE(TAG, "Failed to create timer");
    }
}

void timer_start(uint32_t minutes)
{
    if (relay_timer_handle == NULL)
    {
        ESP_LOGE(TAG, "Timer not initialized!");
        return;
    }

    if (minutes == 0)
        return;

    // Chuyển phút sang tick của FreeRTOS
    total_ticks_on_start = pdMS_TO_TICKS(minutes * 60 * 1000);
    start_tick = xTaskGetTickCount();

    // Thay đổi thời gian của timer và khởi động nó
    if (xTimerChangePeriod(relay_timer_handle, total_ticks_on_start, 0) == pdPASS)
    {
        xTimerStart(relay_timer_handle, 0);
        relay_set(1); // Bật relay
        ESP_LOGI(TAG, "Timer started for %lu minutes. Relay is ON.", minutes);
        oled_update(OLED_STATE_ON_TIMER, minutes * 60);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to start timer");
    }
}

void timer_stop()
{
    // Dừng timer tắt relay nếu nó đang chạy
    if (relay_timer_handle != NULL && xTimerIsTimerActive(relay_timer_handle))
    {
        xTimerStop(relay_timer_handle, 0);
        relay_set(0);
        total_ticks_on_start = 0;
        oled_update(OLED_STATE_OFF, 0);
        ESP_LOGI(TAG, "Timer stopped. Relay is OFF.");
    }
}

int timer_get_remaining_seconds()
{
    if (timer_is_active())
    {
        TickType_t elapsed_ticks = xTaskGetTickCount() - start_tick;
        if (elapsed_ticks >= total_ticks_on_start)
            return 0;
        TickType_t remaining_ticks = total_ticks_on_start - elapsed_ticks;
        return (int)(remaining_ticks / configTICK_RATE_HZ);
    }
    return 0;
}

bool timer_is_active()
{
    if (relay_timer_handle == NULL)
    {
        return false;
    }
    // xTimerIsTimerActive trả về pdTRUE hoặc pdFALSE
    return xTimerIsTimerActive(relay_timer_handle) == pdTRUE;
}