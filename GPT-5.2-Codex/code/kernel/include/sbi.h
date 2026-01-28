#pragma once

#include <stdint.h>

void sbi_set_timer(uint64_t stime_value);

#define SBI_SRST_RESET_TYPE_SHUTDOWN 0u
#define SBI_SRST_RESET_TYPE_COLD_REBOOT 1u
#define SBI_SRST_RESET_TYPE_WARM_REBOOT 2u
#define SBI_SRST_REASON_NONE 0u

void sbi_system_reset(uint32_t reset_type, uint32_t reset_reason);
