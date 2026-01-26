#pragma once

#include <stdint.h>

struct bootinfo;

void timer_init(uint64_t hartid, const struct bootinfo *bi);
void timer_handle(uint64_t hartid);
uint64_t timer_ticks(void);
