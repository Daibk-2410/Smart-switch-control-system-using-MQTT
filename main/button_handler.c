// main/button_handler.c (Phiên bản chống dội nâng cao)

#include "button_handler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "relay_control.h"
#include "timer_control.h"
#include <stdbool.h>

#define TOGGLE_BUTTON_PIN 32
#define TIMER_BUTTON_PIN 33
#define CANCEL_TIMER_BUTTON_PIN 25

static const char *TAG = "BUTTON";

// --- Logic hẹn giờ ---
static int timer_press_count = 0;
static TimerHandle_t multi_press_timer = NULL;

// --- Logic chống dội nâng cao ---
static TimerHandle_t debounce_timer = NULL;
static volatile uint32_t last_pressed_pin = 0;

// Hàm xử lý sự kiện nút bấm sau khi đã chống dội
void handle_button_press(uint32_t pin)
{

    if (pin == TOGGLE_BUTTON_PIN)
    {
        if (timer_is_active())
        {
            ESP_LOGW(TAG, "Timer is active. Please cancel it first to use toggle button.");
        }
        else
        {
            ESP_LOGI(TAG, "Action: Toggle Relay");
            relay_toggle();
        }
    }

    if (pin == TIMER_BUTTON_PIN)
    {
        vTaskDelay(50);
        if (pin == TIMER_BUTTON_PIN)
        {

            if (relay_get_state() == 1)
            {
                ESP_LOGW(TAG, "Relay is already ON. Please turn it off to set a timer.");
            }
            else
            {
                timer_press_count++;
                ESP_LOGI(TAG, "Timer button press detected. Count: %d", timer_press_count);
                // Reset "cửa sổ" 2 giây
                xTimerReset(multi_press_timer, 0);
            }
        }
    }

    if (pin == CANCEL_TIMER_BUTTON_PIN)
    {
        vTaskDelay(50);
        if (pin == CANCEL_TIMER_BUTTON_PIN)
        {
            ESP_LOGI(TAG, "Action: Cancel Timer");
            // Reset luôn bộ đếm nếu người dùng đang nhấn dở
            if (timer_press_count > 0)
            {
                timer_press_count = 0;
                ESP_LOGI(TAG, "Multi-press count has been reset.");
            }
            timer_stop();
        }
    }
}

// Callback của timer chống dội
void debounce_timer_callback(TimerHandle_t xTimer)
{
    // Thời gian yên tĩnh đã hết, bây giờ mới xử lý sự kiện
    handle_button_press(last_pressed_pin);
    // Thêm độ trễ nhỏ trước khi kích hoạt lại ngắt
    vTaskDelay(pdMS_TO_TICKS(10));
    // Kích hoạt lại ngắt cho tất cả các nút
    gpio_intr_enable(TOGGLE_BUTTON_PIN);
    gpio_intr_enable(TIMER_BUTTON_PIN);
    gpio_intr_enable(CANCEL_TIMER_BUTTON_PIN);
}

// Callback của timer đa-nhấn
void multi_press_timer_callback(TimerHandle_t xTimer)
{
    if (timer_press_count > 0)
    {
        ESP_LOGI(TAG, "Multi-press window closed. Starting timer for %d minutes.", 10 * timer_press_count);
        timer_start(10 * timer_press_count);
    }
    timer_press_count = 0;
}

// Hàm ngắt (ISR) - cực kỳ gọn nhẹ
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    // Vô hiệu hóa TẤT CẢ các ngắt nút bấm ngay lập tức
    gpio_intr_disable(TOGGLE_BUTTON_PIN);
    gpio_intr_disable(TIMER_BUTTON_PIN);
    gpio_intr_disable(CANCEL_TIMER_BUTTON_PIN);

    // Ghi lại chân nào đã gây ra ngắt
    last_pressed_pin = (uint32_t)arg;

    // Khởi động timer chống dội từ ISR
    xTimerResetFromISR(debounce_timer, NULL);
}

void button_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TOGGLE_BUTTON_PIN) | (1ULL << TIMER_BUTTON_PIN) | (1ULL << CANCEL_TIMER_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_conf);

    // Tạo các timer
    // Timer chống dội 100ms, one-shot
    debounce_timer = xTimerCreate("DebounceTimer", pdMS_TO_TICKS(100), pdFALSE, 0, debounce_timer_callback);
    // Timer đa-nhấn 2 giây, one-shot
    multi_press_timer = xTimerCreate("MultiPressTimer", pdMS_TO_TICKS(2000), pdFALSE, 0, multi_press_timer_callback);

    // Cài đặt dịch vụ ngắt
    gpio_install_isr_service(0);
    gpio_isr_handler_add(TOGGLE_BUTTON_PIN, gpio_isr_handler, (void *)TOGGLE_BUTTON_PIN);
    gpio_isr_handler_add(TIMER_BUTTON_PIN, gpio_isr_handler, (void *)TIMER_BUTTON_PIN);
    gpio_isr_handler_add(CANCEL_TIMER_BUTTON_PIN, gpio_isr_handler, (void *)CANCEL_TIMER_BUTTON_PIN);

    ESP_LOGI(TAG, "Advanced button handler initialized.");
}