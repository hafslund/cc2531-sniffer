#include <stdarg.h>

void log_init();

enum log_level {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO = 1,
  LOG_LEVEL_ERROR = 2
};

#define log_err_null(log, fmt, ...) \
  ({log_msg(log, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__);	\
    NULL;})

#define log_err(log, fmt, ...) \
  ({log_msg(log, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__);	\
    -1;})

struct log {
  struct log_vtable *vtable;
};

struct log_vtable {
  void (*msg)(struct log *, enum log_level level, const char *fmt, va_list ap);
  void (*free)(struct log *);
};

struct log *log_stdio_create();

void log_msg(struct log *log, enum log_level level, const char *fmt, ...);
void log_free(struct log *log);
