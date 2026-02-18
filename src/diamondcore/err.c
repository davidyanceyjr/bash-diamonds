// err.c - shared error utilities

#include "diamondcore.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void dc_err_init(dc_error_t *err) {
  if (!err) return;
  err->code = DC_ERR_NONE;
  err->msg[0] = '\0';
}

void dc_err_set(dc_error_t *err, dc_err_code_t code, const char *fmt, ...) {
  if (!err) return;
  err->code = code;
  err->msg[0] = '\0';
  if (!fmt) return;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(err->msg, sizeof(err->msg), fmt, ap);
  va_end(ap);
  // Ensure NUL termination even on older libc.
  err->msg[sizeof(err->msg) - 1] = '\0';
}

int dc_exit_code_from_error(const dc_error_t *err) {
  if (!err) return 2;
  switch (err->code) {
  case DC_ERR_NONE:
    return 0;
  case DC_ERR_USAGE:
    return 2;
  case DC_ERR_IO:
  case DC_ERR_NOMEM:
  case DC_ERR_INTERNAL:
  default:
    return 2;
  }
}
