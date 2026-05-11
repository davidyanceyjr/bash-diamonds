/* src/include/diamondcore.h */
/* DROP-IN: restore original header structure and add replace declarations only
 */

#ifndef DIAMONDCORE_H
#define DIAMONDCORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* =============================================================================
 * Errors
 *
 * Keep these names/signatures stable: multiple core modules include this header
 * and -Werror will turn any drift into hard failures.
 * =============================================================================
 */

typedef enum {
  DC_ERR_NONE = 0,
  DC_ERR_USAGE,
  DC_ERR_IO,
  DC_ERR_NOMEM,
  DC_ERR_INTERNAL,
} dc_err_code_t;

typedef struct dc_sel dc_sel_t;
typedef struct dc_line_reader dc_line_reader_t;

typedef struct {
  dc_err_code_t code;
  char msg[256];
} dc_error_t;

typedef struct {
  const uint8_t *ptr;
  size_t len;
  bool ends_with_nl;
} dc_line_view_t;

/* Field splitting */

typedef struct {
  const uint8_t *ptr;
  size_t len;
} dc_field_view_t;

/* Error helpers (diamondcore/err.c) */
void dc_err_init(dc_error_t *err);

void dc_err_set(dc_error_t *err, dc_err_code_t code, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

int dc_exit_code_from_error(const dc_error_t *err);

/* =============================================================================
 * Builtin usage + help printers
 *
 * - usage printers: one-line usage (also used on error paths)
 * - help printers: full help for --help (stdout, exit 0)
 * =============================================================================
 */

/* Builtin-specific one-line usage printers */
void dc_print_usage_trim(FILE *out);
void dc_print_usage_lines(FILE *out);
void dc_print_usage_fields(FILE *out);
void dc_print_usage_match(FILE *out);
void dc_print_usage_replace(FILE *out); /* NEW */
void dc_print_usage_take(FILE *out);
void dc_print_usage_table(FILE *out);
void dc_print_usage_count(FILE *out);
void dc_print_usage_filter(FILE *out);

/* Builtin-specific full help printers */
void dc_print_help_trim(FILE *out);
void dc_print_help_lines(FILE *out);
void dc_print_help_fields(FILE *out);
void dc_print_help_match(FILE *out);
void dc_print_help_replace(FILE *out); /* NEW */
void dc_print_help_take(FILE *out);
void dc_print_help_count(FILE *out);
void dc_print_help_table(FILE *out);
void dc_print_help_filter(FILE *out);

/* Shared help fragments (diamondcore/help_common.c) */
void dc_print_help_common_exit_codes(FILE *out);
void dc_print_help_common_files(FILE *out);
void dc_print_help_common_sigpipe(FILE *out);

/* Shared range SPEC grammar text used by lines/fields help and docs.
 * - what: header label (e.g., "SPEC")
 * - scope: one-line scope description (may be "")
 */
void dc_print_help_common_range_spec(FILE *out, const char *what,
                                     const char *scope);

/* =============================================================================
 * Strict unsigned base-10 integer parsing.
 *
 * - digits only
 * - no sign
 * - no leading zeros unless exactly "0"
 * - must fit in uint64_t
 *
 * On failure sets err->code=DC_ERR_USAGE and a short message (e.g. "invalid
 * N").
 * =============================================================================
 */
bool dc_parse_u64_dec_strict(const char *s, uint64_t *out, const char *label,
                             dc_error_t *err);

/* =============================================================================
 * Selection (range parser + normalizer)
 * =============================================================================
 */
dc_sel_t *dc_sel_parse_and_normalize(const char *spec, dc_error_t *err);
bool dc_sel_wants(dc_sel_t *sel, uint64_t line_no);
uint64_t dc_sel_max_finite(dc_sel_t *sel, bool *has_max);
void dc_sel_free(dc_sel_t *sel);

/* =============================================================================
 * Line reader (streaming, bytewise)
 * =============================================================================
 */
dc_line_reader_t *dc_lr_open(char *const *files, size_t file_count,
                             dc_error_t *err);
bool dc_lr_next(dc_line_reader_t *lr, dc_line_view_t *out, dc_error_t *err);
void dc_lr_close(dc_line_reader_t *lr);

/* =============================================================================
 * Field splitting
 * =============================================================================
 */

/* Split a line into non-empty fields separated by ASCII whitespace.
 * - Returns number of fields.
 * - On success, *out_fields points to heap array of views into line buffer (no
 * copies). Caller free().
 * - If no fields, returns 0 and sets *out_fields = NULL.
 * - On allocation failure, returns (size_t)-1 and sets *out_fields = NULL.
 * - Trailing '\n' is treated as whitespace.
 */
size_t dc_split_ws(const uint8_t *line, size_t len,
                   dc_field_view_t **out_fields);

/* Split a line into fields separated by a single byte delimiter.
 * - Preserves empty fields (consecutive/leading/trailing delimiters).
 * - Returns number of fields (>=1 for any line, including empty).
 * - On success, *out_fields points to heap array of views into line buffer (no
 * copies). Caller free().
 * - On allocation failure, returns (size_t)-1 and sets *out_fields = NULL.
 */
size_t dc_split_delim(const uint8_t *line, size_t len, uint8_t delim,
                      dc_field_view_t **out_fields);

#endif /* DIAMONDCORE_H */