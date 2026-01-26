#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>

#define TICK_HZ 100

void timer_init(void);
void timer_handle(void);
uint64_t get_ticks(void);

#endif
