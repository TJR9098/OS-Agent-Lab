#ifndef _SBI_H
#define _SBI_H

#include <stdint.h>

struct sbi_ret {
    long error;
    long value;
};

struct sbi_ret sbi_call(long eid, long fid, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5);

void sbi_console_putchar(int ch);
void sbi_set_timer(uint64_t stime);
void sbi_shutdown(void);

#define SBI_EXT_TIME 0x54494D45
#define SBI_EXT_RFENCE 0x52464345
#define SBI_EXT_SRST 0x53525354

#define SBI_ERR_SUCCESS 0
#define SBI_ERR_FAILED -1
#define SBI_ERR_NOT_SUPPORTED -2
#define SBI_ERR_INVALID_PARAM -3
#define SBI_ERR_DENIED -4
#define SBI_ERR_ALREADY_STARTED -5
#define SBI_ERR_ALREADY_STOPPED -6
#define SBI_ERR_NO_SHM -7
#define SBI_ERR_BUSY -8
#define SBI_ERR_IRQPending -9

#define SRST_SHUTDOWN 0x00000000
#define SRST_REBOOT 0x00000001

#endif
