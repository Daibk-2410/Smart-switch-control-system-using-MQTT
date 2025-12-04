#include "relay_control.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "mqtt_manager.h"
#include "timer_control.h"
#include "oled_display.h"

static int relay_pin = -1;
static int current_state = 0;

void relay_init(int gpio_num)
{
    relay_pin = gpio_num;
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << relay_pin),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    gpio_set_level(relay_pin, 0);
}

void relay_toggle(void)
{
    relay_set(!current_state);
    ESP_LOGI("RELAY", "Relay toggled to %s", current_state ? "ON" : "OFF");

    // Khi bật/tắt thủ công, màn hình sẽ hiển thị trạng thái tương ứng
    if (current_state == 1)
    {
        oled_update(OLED_STATE_ON_INDEFINITE, 0);
    }
    else
    {
        oled_update(OLED_STATE_OFF, 0);
    }
}

int relay_get_state(void)
{
    return current_state;
}

void relay_set(int state)
{
    if (relay_pin != -1)
    {
        gpio_set_level(relay_pin, state);
        current_state = state;
        mqtt_publish("home/relay1/status", current_state ? "ON" : "OFF");
    }
}
