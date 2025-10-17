#include "relay_control.h"
#include "driver/gpio.h"

static int relay_pin = -1;

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

void relay_set(int state)
{
    if (relay_pin != -1)
        gpio_set_level(relay_pin, state);
}
