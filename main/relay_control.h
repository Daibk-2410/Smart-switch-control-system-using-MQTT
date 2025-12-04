#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

void relay_init(int gpio_num);
void relay_set(int state);
void relay_toggle(void);
int relay_get_state(void);

#endif
