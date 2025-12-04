#ifndef TIMER_CONTROL_H
#define TIMER_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

void timer_init();
void timer_start(uint32_t minutes);
void timer_stop();
int timer_get_remaining_seconds();
bool timer_is_active(void);
#endif