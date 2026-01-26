#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// 初始化定时器
void timer_init(void);

// 定时器中断处理
void timer_handle(void);

// 获取当前tick数
uint64_t timer_get_tick(void);

#endif // TIMER_H