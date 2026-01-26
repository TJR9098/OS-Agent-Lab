#include "sbi.h"

struct sbi_ret sbi_call(long eid, long fid, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5) {
    struct sbi_ret ret;
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    asm volatile(
        "ecall"
        : "=r"(a0), "=r"(a1)
        : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
        : "memory"
    );

    ret.error = a0;
    ret.value = a1;
    return ret;
}

void sbi_console_putchar(int ch) {
    sbi_call(0x1, 0, ch, 0, 0, 0, 0, 0);
}

void sbi_set_timer(uint64_t stime) {
    sbi_call(SBI_EXT_TIME, 0, stime, 0, 0, 0, 0, 0);
}

void sbi_shutdown(void) {
    struct sbi_ret ret = sbi_call(SBI_EXT_SRST, 0, SRST_SHUTDOWN, 0, 0, 0, 0, 0);
    
    if (ret.error != SBI_ERR_SUCCESS && ret.error != SBI_ERR_NOT_SUPPORTED) {
        ret = sbi_call(0x08, 0, 0, 0, 0, 0, 0, 0);
    }
    
    asm volatile(
        "csrw sstatus, zero\n\t"
        "wfi"
        : : : "memory"
    );
    
    while (1) {
        asm volatile("wfi" : : : "memory");
    }
}
