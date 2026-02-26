// diamondcore.h
//
// Common core utilities for Diamond builtins.
// This header is shared by builtins and diamondcore implementation.

#ifndef DIAMONDCORE_H
#define DIAMONDCORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* -----------------------------
 * Error model (simple + stable)
 * ----------------------------- */

typedef enum {
  DC_ERR_NONE = 0,
  DC_ERR_USAGE,
  DC_ERR_IO,
  DC_ERR_NOMEM,
  DC_ERR_PARSE,
} dc_errcode_t;

typedef struct {
  dc_errcode_t code;
  char msg[256];
} dc_error_t;

/* -----------------------------
 * Line reader (streaming)
 * ----------------------------- */

typedef struct dc_line_reader dc_line_reader_t;

typedef struct {
  const uint8_t *ptr;
  size_t len;         // includes '\n' if present
  bool ends_with_nl;  // whether the original line ended with '\n'
} dc_line_view_t;

dc_line_reader_t *dc_lr_open(char *const *files, size_t file_count, dc_error_t *err);
bool dc_lr_next(dc_line_reader_t *lr, dc_line_view_t *out, dc_error_t *err);
void dc_lr_close(dc_line_reader_t *lr);

/* -----------------------------
 * Range selection (SPEC)
 * ----------------------------- */

typedef struct dc_sel dc_sel_t;

dc_sel_t *dc_sel_parse_and_normalize(const char *spec, dc_error_t *err);
bool dc_sel_wants(const dc_sel_t *sel, uint64_t idx);
uint64_t dc_sel_max_finite(const dc_sel_t *sel, bool *has_max);
void dc_sel_free(dc_sel_t *sel);

/* -----------------------------
 * Splitting helpers (fields)
 * ----------------------------- */

typedef struct {
  const uint8_t *ptr;
  size_t len;
} dc_field_view_t;

size_t dc_split_ws(const uint8_t *buf, size_t len, dc_field_view_t **out_fields);
size_t dc_split_delim(const uint8_t *buf, size_t len, uint8_t delim, dc_field_view_t **out_fields);

/* -----------------------------
 * Strict uint parsing (take)
 * ----------------------------- */

bool dc_parse_u64_dec_strict(const char *s, uint64_t *out, const char *label, dc_error_t *err);

/* -----------------------------
 * Builtin-specific usage printers
 * ----------------------------- */

void dc_print_usage_trim(FILE *out);
void dc_print_usage_lines(FILE *out);
void dc_print_usage_fields(FILE *out);
void dc_print_usage_match(FILE *out);
void dc_print_usage_take(FILE *out);

/* Builtin-specific help printers (for --help). */
void dc_print_help_trim(FILE *out);
void dc_print_help_lines(FILE *out);
void dc_print_help_fields(FILE *out);
void dc_print_help_match(FILE *out);
void dc_print_help_take(FILE *out);

/* Shared help fragments. */
void dc_print_help_common_exit_codes(FILE *out);
void dc_print_help_common_files(FILE *out);
void dc_print_help_common_sigpipe(FILE *out);
void dc_print_help_common_range_spec(FILE *out, const char *what, const char *scope);

#endif // DIAMONDCORE_H