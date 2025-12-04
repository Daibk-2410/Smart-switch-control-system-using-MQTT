#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    OLED_STATE_OFF,
    OLED_STATE_ON_INDEFINITE,
    OLED_STATE_ON_TIMER
} oled_display_state_t;

void oled_init(void);
void oled_update(oled_display_state_t state, int remaining_seconds);

#endif