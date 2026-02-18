// split.c - ASCII whitespace field splitting utilities

#include "diamondcore.h"

#include <stdlib.h>

static inline bool is_ws(uint8_t c) {
  // ASCII whitespace used by spec:
  // space, tab, newline, carriage return, vertical tab, form feed
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
          c == '\f');
}

size_t dc_split_ws(const uint8_t *line, size_t len, dc_field_view_t **out_fields) {
  if (out_fields) *out_fields = NULL;
  if (!out_fields || (!line && len != 0)) return 0;

  dc_field_view_t *v = NULL;
  size_t cap = 0;
  size_t cnt = 0;

  size_t i = 0;
  while (i < len) {
    // Skip whitespace.
    while (i < len && is_ws(line[i])) i++;
    if (i >= len) break;

    // Start of field.
    size_t start = i;
    while (i < len && !is_ws(line[i])) i++;
    size_t flen = i - start;
    if (flen == 0) continue;

    if (cnt == cap) {
      size_t ncap = cap ? cap * 2 : 8;
      dc_field_view_t *nv = (dc_field_view_t *)realloc(v, ncap * sizeof(*nv));
      if (!nv) {
        free(v);
        *out_fields = NULL;
        return (size_t)-1;
      }
      v = nv;
      cap = ncap;
    }

    v[cnt].ptr = line + start;
    v[cnt].len = flen;
    cnt++;
  }

  if (cnt == 0) {
    free(v);
    *out_fields = NULL;
    return 0;
  }

  *out_fields = v;
  return cnt;
}
