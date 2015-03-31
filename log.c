#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "log.h"

void log_msg(struct log *log, enum log_level level, const char *fmt, ...) {
  if (level >= log->level) {
    va_list ap;
    va_start(ap, fmt);
    log->vtable->msg(log, level, fmt, ap);
    va_end(ap);
  }
}

void log_free(struct log *log) {
  return log->vtable->free(log);
}

/* STDIO log */

struct log_stdio {
  struct log log;
};

static void
log_stdio_msg(struct log *l, enum log_level level, const char *fmt, va_list ap) {
  FILE *stream;

  if (level == LOG_LEVEL_ERROR) stream = stderr; else stream = stdout;
  vfprintf(stream, fmt, ap);
  fprintf(stream, "\n");
}

static void
log_stdio_free(struct log *l) {
  free(l);
}

struct log_vtable log_stdio_vtable = {
  log_stdio_msg, log_stdio_free
};

struct log *
log_stdio_create(enum log_level level) {
  struct log_stdio *ls = malloc(sizeof(*ls));
  ls->log.level = level;
  ls->log.vtable = &log_stdio_vtable;
  return &ls->log;
}

