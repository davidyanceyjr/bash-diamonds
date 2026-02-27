// builtin_table.c - `table` loadable builtin

#include "diamondcore.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Bash loadable builtin headers (vendored minimal subset for loadables)
#include "builtins.h"
#include "shell.h"

__attribute__((unused))
static const char *table_shortdoc = "table [--] [FILE...]";

static char *table_doc[] = {
    "Format delimited text into aligned columns for human output.",
    (char *)0,
};

// === ANCHOR:ERROR-HELPERS-BEGIN ===
static int table_usage_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "table: %s\n", msg);
  else dc_print_usage_table(stderr);
  return 2;
}

static int table_io_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "table: %s\n", msg);
  else fprintf(stderr, "table: I/O error\n");
  return 2;
}

static int table_help(void) {
  dc_print_help_table(stdout);
  return 0;
}
// === ANCHOR:ERROR-HELPERS-END ===

static inline bool is_st_ws(uint8_t c) { return (c == ' ' || c == '\t'); }

// Split a line into non-empty fields separated by runs of space/tab.
// - Leading/trailing space/tab ignored.
// - Returns number of fields; caller free()s *out_fields.
// - On allocation failure returns (size_t)-1.
static size_t table_split_space_tab(const uint8_t *line, size_t len,
                                   dc_field_view_t **out_fields) {
  if (out_fields) *out_fields = NULL;
  if (!out_fields || (!line && len != 0)) return 0;

  dc_field_view_t *v = NULL;
  size_t cap = 0;
  size_t cnt = 0;

  size_t i = 0;
  while (i < len) {
    while (i < len && is_st_ws(line[i])) i++;
    if (i >= len) break;

    size_t start = i;
    while (i < len && !is_st_ws(line[i])) i++;
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

// Separator rule implied by docs example:
// - after column 0 (between col1 and col2): 1 space minimum
// - after columns >=1 (between col2 and later): 2 spaces minimum
static inline size_t table_min_sep(size_t col_index) {
  return (col_index == 0) ? 1u : 2u;
}

static int table_nonseekable_err(void) {
  // Required exact message.
  fprintf(stderr, "table: non-seekable input not supported\n");
  return 2;
}

static bool table_check_seekable_files(char *const *files, size_t file_count,
                                      char *errbuf, size_t errcap) {
  if (errbuf && errcap) errbuf[0] = '\0';

  if (file_count == 0) return false;

  for (size_t i = 0; i < file_count; i++) {
    const char *name = files[i];
    if (!name) name = "";

    if (strcmp(name, "-") == 0) return false;

    struct stat st;
    if (stat(name, &st) != 0) {
      if (errbuf && errcap) {
        snprintf(errbuf, errcap, "cannot open '%s': %s", name, strerror(errno));
      }
      return false;
    }

    if (!S_ISREG(st.st_mode)) return false;
  }

  return true;
}

static int table_pass_compute_widths(char *const *files, size_t file_count,
                                    size_t **out_widths, size_t *out_ncols,
                                    bool *out_any) {
  if (out_widths) *out_widths = NULL;
  if (out_ncols) *out_ncols = 0;
  if (out_any) *out_any = false;

  dc_error_t err;
  dc_line_reader_t *lr = dc_lr_open(files, file_count, &err);
  if (!lr) return table_io_err(err.msg[0] ? err.msg : "cannot open input");

  size_t *widths = NULL;
  size_t ncols = 0;
  bool any = false;

  for (;;) {
    dc_line_view_t v;
    bool ok = dc_lr_next(lr, &v, &err);
    if (!ok) {
      if (err.code != DC_ERR_NONE) {
        dc_lr_close(lr);
        free(widths);
        return table_io_err(err.msg[0] ? err.msg : "read error");
      }
      break; // EOF
    }

    const uint8_t *line = v.ptr;
    size_t linelen = v.len;
    if (v.ends_with_nl && linelen > 0) linelen--; // exclude '\n'

    dc_field_view_t *fields = NULL;
    size_t nf = table_split_space_tab(line, linelen, &fields);
    if (nf == (size_t)-1) {
      dc_lr_close(lr);
      free(widths);
      return table_io_err("out of memory");
    }
    if (nf == 0) {
      free(fields);
      continue;
    }

    any = true;

    if (nf > ncols) {
      size_t *nw = (size_t *)realloc(widths, nf * sizeof(size_t));
      if (!nw) {
        free(fields);
        dc_lr_close(lr);
        free(widths);
        return table_io_err("out of memory");
      }
      for (size_t i = ncols; i < nf; i++) nw[i] = 0;
      widths = nw;
      ncols = nf;
    }

    for (size_t i = 0; i < nf; i++) {
      if (fields[i].len > widths[i]) widths[i] = fields[i].len;
    }

    free(fields);
  }

  dc_lr_close(lr);

  if (out_widths) *out_widths = widths;
  else free(widths);

  if (out_ncols) *out_ncols = ncols;
  if (out_any) *out_any = any;

  return 0;
}

static int table_pass_emit(char *const *files, size_t file_count,
                           const size_t *widths, size_t ncols,
                           bool *out_any) {
  if (out_any) *out_any = false;

  dc_error_t err;
  dc_line_reader_t *lr = dc_lr_open(files, file_count, &err);
  if (!lr) return table_io_err(err.msg[0] ? err.msg : "cannot open input");

  bool any = false;

  for (;;) {
    dc_line_view_t v;
    bool ok = dc_lr_next(lr, &v, &err);
    if (!ok) {
      if (err.code != DC_ERR_NONE) {
        dc_lr_close(lr);
        return table_io_err(err.msg[0] ? err.msg : "read error");
      }
      break; // EOF
    }

    const uint8_t *line = v.ptr;
    size_t linelen = v.len;
    if (v.ends_with_nl && linelen > 0) linelen--; // exclude '\n'

    dc_field_view_t *fields = NULL;
    size_t nf = table_split_space_tab(line, linelen, &fields);
    if (nf == (size_t)-1) {
      dc_lr_close(lr);
      return table_io_err("out of memory");
    }
    if (nf == 0) {
      free(fields);
      continue;
    }

    any = true;

    for (size_t i = 0; i < nf; i++) {
      const uint8_t *p = fields[i].ptr;
      size_t n = fields[i].len;

      if (n > 0) {
        size_t w = fwrite(p, 1, n, stdout);
        if (w != n) {
          free(fields);
          dc_lr_close(lr);
          return table_io_err("write error");
        }
      }

      if (i + 1 < nf) {
        size_t colw = (i < ncols) ? widths[i] : 0;
        size_t minsep = table_min_sep(i);
        size_t pad = minsep;
        if (colw > n) pad += (colw - n);
        for (size_t s = 0; s < pad; s++) {
          if (fputc(' ', stdout) == EOF) {
            free(fields);
            dc_lr_close(lr);
            return table_io_err("write error");
          }
        }
      }
    }

    if (v.ends_with_nl) {
      if (fputc('\n', stdout) == EOF) {
        free(fields);
        dc_lr_close(lr);
        return table_io_err("write error");
      }
    }

    free(fields);
  }

  dc_lr_close(lr);

  if (out_any) *out_any = any;
  return 0;
}

// === ANCHOR:CORE-MAIN-BEGIN ===
static int table_main(char *const *files, size_t file_count) {
  char errbuf[256];
  if (!table_check_seekable_files(files, file_count, errbuf, sizeof(errbuf))) {
    if (errbuf[0]) return table_io_err(errbuf);
    return table_nonseekable_err();
  }

  size_t *widths = NULL;
  size_t ncols = 0;
  bool any1 = false;

  int rc = table_pass_compute_widths(files, file_count, &widths, &ncols, &any1);
  if (rc != 0) {
    free(widths);
    return rc;
  }

  if (!any1) {
    free(widths);
    return 1;
  }

  bool any2 = false;
  rc = table_pass_emit(files, file_count, widths, ncols, &any2);
  free(widths);
  if (rc != 0) return rc;
  return any2 ? 0 : 1;
}
// === ANCHOR:CORE-MAIN-END ===

// Parse rules:
// - Recognizes --help and -- (end option parsing).
// - Any other -x token before -- is an error unless token is exactly '-'.
__attribute__((visibility("default")))
int table_builtin(WORD_LIST *list) {
  // === ANCHOR:SIGPIPE-BEGIN ===
  void (*old_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
  // === ANCHOR:SIGPIPE-END ===

  bool end_opts = false;

  size_t fcap = 8;
  size_t fcnt = 0;
  char **files = (char **)calloc(fcap, sizeof(char *));
  if (!files) {
    signal(SIGPIPE, old_sigpipe);
    return table_io_err("out of memory");
  }

  int rc = 2;

  for (WORD_LIST *w = list; w; w = w->next) {
    const char *tok = w->word->word;
    if (!tok) tok = "";

    if (!end_opts && strcmp(tok, "--help") == 0) {
      rc = table_help();
      goto out;
    }

    if (!end_opts && strcmp(tok, "--") == 0) {
      end_opts = true;
      continue;
    }

    if (!end_opts && tok[0] == '-' && tok[1] != '\0' && strcmp(tok, "-") != 0) {
      rc = table_usage_err("unknown option (use --help)");
      goto out;
    }

    if (fcnt == fcap) {
      size_t ncap = fcap * 2;
      char **nf = (char **)realloc(files, ncap * sizeof(char *));
      if (!nf) {
        rc = table_io_err("out of memory");
        goto out;
      }
      files = nf;
      fcap = ncap;
    }

    files[fcnt++] = (char *)tok;
  }

  rc = table_main(files, fcnt);

out:
  free(files);
  signal(SIGPIPE, old_sigpipe);
  return rc;
}

__attribute__((visibility("default")))
struct builtin table_struct = {
    .name = "table",
    .function = table_builtin,
    .flags = BUILTIN_ENABLED,
    .long_doc = table_doc,
    .short_doc = (char *)"table [--] [FILE...]",
    .handle = 0,
};