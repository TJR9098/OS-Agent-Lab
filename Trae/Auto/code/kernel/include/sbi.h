#ifndef __SBI_H__
#define __SBI_H__

#include <stdint.h>

// SBI调用返回结果结构
struct sbi_ret {
    long error;
    long value;
};

// 通用SBI调用函数
struct sbi_ret sbi_call(long eid, long fid, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5);

// Console Putchar (legacy)
void sbi_console_putchar(int ch);

// Timer相关
void sbi_set_timer(uint64_t stime);

// 系统关机
void sbi_shutdown(void);

#endif /* __SBI_H__ */
