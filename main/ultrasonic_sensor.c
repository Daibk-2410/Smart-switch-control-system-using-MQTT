// main/ultrasonic_sensor.c
#include "ultrasonic_sensor.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static int TRIGGER_PIN;
static int ECHO_PIN;

void ultrasonic_init(int trigger_pin, int echo_pin) {
    TRIGGER_PIN = trigger_pin;
    ECHO_PIN = echo_pin;

    gpio_config_t trig_conf = {
        .pin_bit_mask = (1ULL << TRIGGER_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&trig_conf);

    gpio_config_t echo_conf = {
        .pin_bit_mask = (1ULL << ECHO_PIN),
        .mode = GPIO_MODE_INPUT,
    };
    gpio_config(&echo_conf);
}

float ultrasonic_get_distance_cm() {
    // 1. Gửi một xung 10 micro-giây đến chân Trigger
    gpio_set_level(TRIGGER_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1)); // Chờ ổn định
    gpio_set_level(TRIGGER_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIGGER_PIN, 0);

    // 2. Chờ chân Echo chuyển sang mức cao (bắt đầu nhận tín hiệu)
    while (gpio_get_level(ECHO_PIN) == 0);
    int64_t start_time = esp_timer_get_time();

    // 3. Chờ chân Echo chuyển về mức thấp (kết thúc nhận tín hiệu)
    while (gpio_get_level(ECHO_PIN) == 1);
    int64_t end_time = esp_timer_get_time();

    // 4. Tính toán khoảng cách
    int64_t duration = end_time - start_time;
    // Khoảng cách (cm) = (Thời gian (us) * Tốc độ âm thanh (cm/us)) / 2
    // Tốc độ âm thanh ~ 343 m/s = 0.0343 cm/us
    float distance = 100 - (duration * 0.0343) / 2.0;

    return distance;
}