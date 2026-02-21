#ifndef DC_REGEX_H
#define DC_REGEX_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Spec resource limits */
#define DC_REGEX_MAX_PATTERN_LEN      4096
#define DC_REGEX_MAX_PROG_INSN        16384
#define DC_REGEX_MAX_ACTIVE_STATES    8192
#define DC_REGEX_MAX_STEPS            2000000

typedef struct dc_regex dc_regex_t;

/* Compile PATTERN once; empty pattern is valid. */
bool dc_regex_compile(dc_regex_t **out_re,
                      const char *pattern,
                      char errbuf[256]);

void dc_regex_free(dc_regex_t *re);

/* Subject does NOT include newline. */
bool dc_regex_match_line(const dc_regex_t *re,
                         const uint8_t *subject,
                         size_t subject_len,
                         bool *exec_limit_exceeded);

#ifdef __cplusplus
}
#endif

#endif
