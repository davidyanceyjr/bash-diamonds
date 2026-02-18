// src/diamondcore/range.c
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "diamondcore.h"

typedef struct {
  uint64_t start;
  uint64_t end; /* UINT64_MAX means open-ended */
} dc_range_t;

struct dc_sel {
  dc_range_t *ranges;
  size_t      nranges;
  size_t      cap;

  /* Streaming cursor: monotone line_no => monotone range index */
  size_t cursor;
};

static inline bool is_ws(char c) { return c == ' ' || c == '\t'; }

static void skip_ws(const char **p) {
  while (p && *p && **p && is_ws(**p)) (*p)++;
}

static bool add_range(dc_sel_t *sel, uint64_t a, uint64_t b, dc_error_t *err) {
  if (!sel) return false;
  if (sel->nranges == sel->cap) {
    size_t newcap = sel->cap ? sel->cap * 2 : 8;
    void *np = realloc(sel->ranges, newcap * sizeof(dc_range_t));
    if (!np) {
      dc_err_set(err, DC_ERR_NOMEM, "lines: out of memory");
      return false;
    }
    sel->ranges = (dc_range_t *)np;
    sel->cap = newcap;
  }
  sel->ranges[sel->nranges++] = (dc_range_t){ .start = a, .end = b };
  return true;
}

/* Strict UINT:
 * - digits only
 * - no leading zeros ("01" invalid)
 * - must be >= 1
 * - must fit uint64_t
 */
static bool parse_uint_strict(const char **p, uint64_t *out, dc_error_t *err) {
  const char *s = *p;
  if (!s || !*s) {
    dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
    return false;
  }
  if (*s < '0' || *s > '9') {
    dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
    return false;
  }
  if (*s == '0') {
    /* either "0" or leading zero like "01" => invalid */
    dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
    return false;
  }

  uint64_t v = 0;
  for (; *s >= '0' && *s <= '9'; s++) {
    uint64_t d = (uint64_t)(*s - '0');
    if (v > (UINT64_MAX - d) / 10) {
      dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
      return false;
    }
    v = v * 10 + d;
  }

  /* No whitespace inside UINT; caller handles allowed whitespace at boundaries. */
  *out = v;
  *p = s;
  return true;
}

static bool match_dots(const char **p) {
  const char *s = *p;
  if (s && s[0] == '.' && s[1] == '.') {
    *p = s + 2;
    return true;
  }
  return false;
}

/*
 * ITEM := INDEX | RANGE
 * INDEX := UINT
 * RANGE := START ".." END
 * START := UINT | ε
 * END   := UINT | ε
 *
 * Whitespace allowed around "," and around ".." only.
 */
static bool parse_item(const char **p, dc_sel_t *sel, dc_error_t *err) {
  skip_ws(p);

  if (!p || !*p || **p == '\0') {
    dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
    return false;
  }

  uint64_t start = 0, end = 0;
  bool have_start = false, have_end = false;

  /* Two cases: (1) starts with ".." => open-start range; (2) starts with UINT */
  if (match_dots(p)) {
    /* "..END" form; START is ε */
    skip_ws(p);
    if (!p || !*p || **p == '\0') {
      /* Bare ".." invalid */
      dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
      return false;
    }
    if (!(**p >= '0' && **p <= '9')) {
      dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
      return false;
    }
    if (!parse_uint_strict(p, &end, err)) return false;
    have_end = true;

    /* Range is 1..end */
    if (end == 0) {
      dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
      return false;
    }
    return add_range(sel, 1, end, err);
  }

  /* Must start with UINT */
  if (!(**p >= '0' && **p <= '9')) {
    dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
    return false;
  }
  if (!parse_uint_strict(p, &start, err)) return false;
  have_start = true;

  /* If it's an INDEX, we're done unless a RANGE follows */
  skip_ws(p);
  if (!match_dots(p)) {
    /* INDEX */
    if (!have_start || start == 0) {
      dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
      return false;
    }
    return add_range(sel, start, start, err);
  }

  /* RANGE: START ".." END (END may be ε) */
  skip_ws(p);
  if (p && *p && **p != '\0') {
    if ((**p >= '0' && **p <= '9')) {
      if (!parse_uint_strict(p, &end, err)) return false;
      have_end = true;
    }
  }

  if (!have_start) {
    dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
    return false;
  }
  if (start == 0) {
    dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
    return false;
  }

  if (!have_end) {
    /* "a.." open-ended */
    return add_range(sel, start, UINT64_MAX, err);
  }

  /* closed "a..b" */
  if (end == 0) {
    dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
    return false;
  }
  if (start > end) {
    dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
    return false;
  }
  return add_range(sel, start, end, err);
}

static int cmp_range(const void *A, const void *B) {
  const dc_range_t *a = (const dc_range_t *)A;
  const dc_range_t *b = (const dc_range_t *)B;
  if (a->start < b->start) return -1;
  if (a->start > b->start) return 1;
  if (a->end < b->end) return -1;
  if (a->end > b->end) return 1;
  return 0;
}

/* Merge ranges in-place; assumes sorted by (start,end). */
static void normalize_ranges(dc_sel_t *sel) {
  if (!sel || sel->nranges == 0) return;

  qsort(sel->ranges, sel->nranges, sizeof(dc_range_t), cmp_range);

  size_t w = 0;
  for (size_t r = 0; r < sel->nranges; r++) {
    dc_range_t cur = sel->ranges[r];
    if (w == 0) {
      sel->ranges[w++] = cur;
      continue;
    }

    dc_range_t *prev = &sel->ranges[w - 1];

    /* Overlap or adjacency merges (dedup) */
    uint64_t prev_end = prev->end;
    uint64_t cur_start = cur.start;

    bool adjacent_or_overlap = false;
    if (prev_end == UINT64_MAX) {
      adjacent_or_overlap = true;
    } else if (cur_start <= prev_end + 1) {
      adjacent_or_overlap = true;
    }

    if (adjacent_or_overlap) {
      /* extend end */
      if (prev->end == UINT64_MAX || cur.end == UINT64_MAX) {
        prev->end = UINT64_MAX;
      } else if (cur.end > prev->end) {
        prev->end = cur.end;
      }
    } else {
      sel->ranges[w++] = cur;
    }
  }

  sel->nranges = w;
  sel->cursor = 0;
}

dc_sel_t *dc_sel_parse_and_normalize(const char *spec, dc_error_t *err) {
  if (err) memset(err,0, sizeof(*err));

  if (!spec || *spec == '\0') {
    dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
    return NULL;
  }

  dc_sel_t *sel = (dc_sel_t *)calloc(1, sizeof(dc_sel_t));
  if (!sel) {
    dc_err_set(err, DC_ERR_NOMEM, "lines: out of memory");
    return NULL;
  }

  const char *p = spec;
  skip_ws(&p);

  /* SPEC := ITEM (',' ITEM)* with optional ws around ',' */
  bool parsed_any = false;

  while (p && *p) {
    /* Reject leading separator */
    skip_ws(&p);
    if (*p == ',') {
      dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
      dc_sel_free(sel);
      return NULL;
    }

    if (!parse_item(&p, sel, err)) {
      dc_sel_free(sel);
      return NULL;
    }
    parsed_any = true;

    skip_ws(&p);

    if (*p == '\0') break;

    if (*p != ',') {
      /* Any junk between items is invalid (incl letters, extra dots, etc.) */
      dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
      dc_sel_free(sel);
      return NULL;
    }

    /* Consume comma, allow ws after comma */
    p++;
    skip_ws(&p);

    /* Trailing separator invalid: "1," */
    if (*p == '\0') {
      dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
      dc_sel_free(sel);
      return NULL;
    }
  }

  if (!parsed_any || sel->nranges == 0) {
    dc_err_set(err, DC_ERR_USAGE, "lines: invalid SPEC");
    dc_sel_free(sel);
    return NULL;
  }

  normalize_ranges(sel);
  return sel;
}

bool dc_sel_wants(dc_sel_t *sel, uint64_t line_no) {
  if (!sel || sel->nranges == 0) return false;

  /* Advance cursor while line_no is beyond current range end */
  while (sel->cursor < sel->nranges) {
    dc_range_t r = sel->ranges[sel->cursor];
    if (line_no < r.start) return false;
    if (r.end == UINT64_MAX) return true;
    if (line_no <= r.end) return true;
    sel->cursor++;
  }
  return false;
}

uint64_t dc_sel_max_finite(dc_sel_t *sel, bool *has_max) {
  if (has_max) *has_max = false;
  if (!sel || sel->nranges == 0) return 0;

  for (size_t i = 0; i < sel->nranges; i++) {
    if (sel->ranges[i].end == UINT64_MAX) {
      if (has_max) *has_max = false;
      return 0;
    }
  }

  uint64_t m = 0;
  for (size_t i = 0; i < sel->nranges; i++) {
    if (sel->ranges[i].end > m) m = sel->ranges[i].end;
  }
  if (has_max) *has_max = true;
  return m;
}

void dc_sel_free(dc_sel_t *sel) {
  if (!sel) return;
  free(sel->ranges);
  free(sel);
}
