#pragma once

#include <stdint.h>

enum log_level {
  LOG_ERROR = 0,
  LOG_WARN = 1,
  LOG_INFO = 2,
  LOG_DEBUG = 3,
  LOG_TRACE = 4,
};

void log_set_level(uint32_t level);
uint32_t log_get_level(void);

void log_write(uint32_t level, const char *fmt, ...);

#define log_error(...) log_write(LOG_ERROR, __VA_ARGS__)
#define log_warn(...)  log_write(LOG_WARN, __VA_ARGS__)
#define log_info(...)  log_write(LOG_INFO, __VA_ARGS__)
#define log_debug(...) log_write(LOG_DEBUG, __VA_ARGS__)
#define log_trace(...) log_write(LOG_TRACE, __VA_ARGS__)