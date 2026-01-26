#include <stdarg.h>
#include <stdint.h>

#include "log.h"
#include "printf.h"

static uint32_t g_log_level = LOG_INFO;

static const char *level_tag(uint32_t level) {
  switch (level) {
    case LOG_ERROR:
      return "ERROR";
    case LOG_WARN:
      return "WARN";
    case LOG_INFO:
      return "INFO";
    case LOG_DEBUG:
      return "DEBUG";
    case LOG_TRACE:
      return "TRACE";
    default:
      return "UNK";
  }
}

void log_set_level(uint32_t level) {
  g_log_level = level;
}

uint32_t log_get_level(void) {
  return g_log_level;
}

void log_write(uint32_t level, const char *fmt, ...) {
  if (level > g_log_level) {
    return;
  }

  kprintf("[%s] ", level_tag(level));

  va_list ap;
  va_start(ap, fmt);
  kvprintf(fmt, ap);
  va_end(ap);

  kprintf("\n");
}