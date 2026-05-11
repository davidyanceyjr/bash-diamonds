#ifndef DC_REGEX_H
#define DC_REGEX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Spec resource limits */
#define DC_REGEX_MAX_PATTERN_LEN 4096
#define DC_REGEX_MAX_PROG_INSN 16384
#define DC_REGEX_MAX_ACTIVE_STATES 8192
#define DC_REGEX_MAX_STEPS 2000000

typedef struct dc_regex dc_regex_t;

/* Compile PATTERN once; empty pattern is valid. */
bool dc_regex_compile(dc_regex_t **out_re, const char *pattern,
                      char errbuf[256]);

void dc_regex_free(dc_regex_t *re);

/* Subject does NOT include newline. */
bool dc_regex_match_line(const dc_regex_t *re, const uint8_t *subject,
                         size_t subject_len, bool *exec_limit_exceeded);

/* Find next match in SUBJECT starting at START_AT.
 * Returns true and sets [*out_start, *out_end) on match.
 * Returns false on no match (or on exec limit exceeded; see flag).
 *
 * This does NOT support capturing; it finds only the overall match span.
 */
bool dc_regex_find_next(const dc_regex_t *re, const uint8_t *subject,
                        size_t subject_len, size_t start_at, size_t *out_start,
                        size_t *out_end, bool *exec_limit_exceeded);

#ifdef __cplusplus
}
#endif

#endif