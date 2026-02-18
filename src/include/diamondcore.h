// diamondcore.h - shared interfaces for Bash Diamonds
//
// Week 1: used by lines (and later builtins). Keep this header small and
// stable. Implementations live under src/diamondcore/.

#ifndef DIAMONDCORE_H
#define DIAMONDCORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// -----------------------------
// Errors
// -----------------------------

typedef enum {
  DC_ERR_NONE = 0,
  DC_ERR_USAGE,
  DC_ERR_IO,
  DC_ERR_NOMEM,
  DC_ERR_INTERNAL,
} dc_err_code_t;

typedef struct {
  dc_err_code_t code;
  char msg[256];
} dc_error_t;

void dc_err_init(dc_error_t *err);
void dc_err_set(dc_error_t *err, dc_err_code_t code, const char *fmt, ...);
int dc_exit_code_from_error(const dc_error_t *err);

// Builtin-specific usage printers (one per builtin)
void dc_print_usage_lines(FILE *out);

// -----------------------------
// Selection (range parser + normalizer)
// -----------------------------

typedef struct dc_sel dc_sel_t;

dc_sel_t *dc_sel_parse_and_normalize(const char *spec, dc_error_t *err);
bool dc_sel_wants(dc_sel_t *sel, uint64_t line_no);
uint64_t dc_sel_max_finite(dc_sel_t *sel, bool *has_max);
void dc_sel_free(dc_sel_t *sel);

// -----------------------------
// Line reader (streaming, bytewise)
// -----------------------------

typedef struct dc_line_reader dc_line_reader_t;

typedef struct {
  const uint8_t *ptr;
  size_t len;
  bool ends_with_nl;
} dc_line_view_t;

dc_line_reader_t *dc_lr_open(char *const *files, size_t file_count, dc_error_t *err);
bool dc_lr_next(dc_line_reader_t *lr, dc_line_view_t *out, dc_error_t *err);
void dc_lr_close(dc_line_reader_t *lr);

#endif // DIAMONDCORE_H
