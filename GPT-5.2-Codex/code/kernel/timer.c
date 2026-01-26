#include <stdint.h>

#include "bootinfo.h"
#include "log.h"
#include "riscv.h"
#include "sbi.h"
#include "timer.h"

static uint64_t g_timer_interval = 0;
static uint32_t g_tick_hz = 0;
static volatile uint64_t g_ticks = 0;
static uint64_t g_next_deadline = 0;

static inline uint64_t read_time(void) {
  return read_csr(time);
}

void timer_init(uint64_t hartid, const struct bootinfo *bi) {
  (void)hartid;
  uint64_t timebase = 10000000ULL;
  uint32_t tick_hz = 100U;

#ifdef CONFIG_STAGE3_DEBUG_BOOT
  log_info("timer: init enter");
#endif

  if (bi) {
    if (bi->timebase_hz != 0) {
      timebase = bi->timebase_hz;
    }
    if (bi->tick_hz != 0) {
      tick_hz = bi->tick_hz;
    }
  }

  g_tick_hz = tick_hz;
  g_timer_interval = timebase / tick_hz;
  if (g_timer_interval == 0) {
    g_timer_interval = 1;
  }
  g_ticks = 0;
#ifdef CONFIG_STAGE3_DEBUG_BOOT
  log_info("timer: before first read_time");
#endif
  g_next_deadline = read_time() + g_timer_interval;
#ifdef CONFIG_STAGE3_DEBUG_BOOT
  log_info("timer: before sbi_set_timer");
#endif
  sbi_set_timer(g_next_deadline);
#ifdef CONFIG_STAGE3_DEBUG_BOOT
  log_info("timer: after sbi_set_timer");
#endif
  log_info("timer: timebase=%lu hz=%u interval=%lu now=%lu next=%lu",
           timebase,
           g_tick_hz,
           g_timer_interval,
           read_time(),
           g_next_deadline);
}

void timer_handle(uint64_t hartid) {
  (void)hartid;
  g_ticks++;
  g_next_deadline += g_timer_interval;
  sbi_set_timer(g_next_deadline);
  if (g_tick_hz != 0 && (g_ticks % g_tick_hz) == 0) {
    log_debug("timer: tick=%lu", g_ticks);
  }
}

uint64_t timer_ticks(void) {
  return g_ticks;
}
