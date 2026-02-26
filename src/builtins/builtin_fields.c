// builtin_fields.c - `fields` loadable builtin

#include "diamondcore.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// Bash loadable builtin headers (vendored minimal subset for loadables)
#include "builtins.h"
#include "shell.h"

__attribute__((unused))
static const char *fields_shortdoc = "fields [-d DELIM] SPEC [--] [FILE...]";

static char *fields_doc[] = {
  "Select and emit specific 1-based fields from each input line.",
  (char *)0,
};

// === ANCHOR:ERROR-HELPERS-BEGIN ===
static int fields_usage_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "fields: %s\n", msg);
  else dc_print_usage_fields(stderr);
  return 2;
}

static int fields_io_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "fields: %s\n", msg);
  else fprintf(stderr, "fields: I/O error\n");
  return 2;
}

static int fields_help(void) {
  dc_print_help_fields(stdout);
  return 0;
}
// === ANCHOR:ERROR-HELPERS-END ===

// === ANCHOR:CORE-MAIN-BEGIN ===
static int fields_main(const char *spec, bool delim_mode, uint8_t delim,
                       char *const *files, size_t file_count) {
  dc_error_t err;
  dc_sel_t *sel = dc_sel_parse_and_normalize(spec, &err);
  if (!sel) {
    return fields_usage_err(err.msg[0] ? err.msg : "invalid SPEC");
  }

  bool has_max = false;
  uint64_t max_finite = dc_sel_max_finite(sel, &has_max);

  dc_line_reader_t *lr = dc_lr_open(files, file_count, &err);
  if (!lr) {
    dc_sel_free(sel);
    return fields_io_err(err.msg[0] ? err.msg : "cannot open input");
  }

  bool emitted_any = false;

  for (;;) {
    dc_line_view_t v;
    bool ok = dc_lr_next(lr, &v, &err);
    if (!ok) {
      if (err.code != DC_ERR_NONE) {
        dc_lr_close(lr);
        dc_sel_free(sel);
        return fields_io_err(err.msg[0] ? err.msg : "read error");
      }
      break; // EOF
    }

    const uint8_t *line = v.ptr;
    size_t linelen = v.len;
    if (v.ends_with_nl && linelen > 0) linelen--; // newline is not part of any field

    dc_field_view_t *fields = NULL;
    size_t nfields = 0;

    if (delim_mode) {
      nfields = dc_split_delim(line, linelen, delim, &fields);
    } else {
      // Whitespace mode: treat newline as whitespace.
      nfields = dc_split_ws(line, v.len, &fields);
    }

    if (nfields == (size_t)-1) {
      dc_lr_close(lr);
      dc_sel_free(sel);
      return fields_io_err("out of memory");
    }
    if (nfields == 0) {
      free(fields);
      continue;
    }

    bool emitted_line = false;
    bool first = true;

    size_t limit = nfields;
    if (has_max && max_finite < (uint64_t)limit) limit = (size_t)max_finite;

    for (size_t i = 0; i < limit; i++) {
      uint64_t idx = (uint64_t)(i + 1);
      if (!dc_sel_wants(sel, idx)) continue;

      if (!first) {
        if (fputc(' ', stdout) == EOF) {
          free(fields);
          dc_lr_close(lr);
          dc_sel_free(sel);
          return fields_io_err("write error");
        }
      }

      const uint8_t *p = fields[i].ptr;
      size_t n = fields[i].len;
      if (n > 0) {
        size_t w = fwrite(p, 1, n, stdout);
        if (w != n) {
          free(fields);
          dc_lr_close(lr);
          dc_sel_free(sel);
          return fields_io_err("write error");
        }
      }

      first = false;
      emitted_line = true;
      emitted_any = true;
    }

    if (emitted_line && v.ends_with_nl) {
      if (fputc('\n', stdout) == EOF) {
        free(fields);
        dc_lr_close(lr);
        dc_sel_free(sel);
        return fields_io_err("write error");
      }
    }

    free(fields);
  }

  dc_lr_close(lr);
  dc_sel_free(sel);
  return emitted_any ? 0 : 1;
}
// === ANCHOR:CORE-MAIN-END ===

// Parsing rules:
// - Only --help and -d DELIM are recognized (before SPEC, unless after --).
// - Any other -x token is an error unless after --, or token is exactly '-'.
// - SPEC is required and is the first non-option token.
__attribute__((visibility("default")))
int fields_builtin(WORD_LIST *list) {
  // === ANCHOR:SIGPIPE-BEGIN ===
  // Ignore SIGPIPE so write failures show up as EPIPE/stdio errors and we return 2.
  void (*old_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
  // === ANCHOR:SIGPIPE-END ===

  bool end_opts = false;
  const char *spec = NULL;

  bool delim_mode = false;
  uint8_t delim = 0;

  size_t fcap = 8;
  size_t fcnt = 0;
  char **files = (char **)calloc(fcap, sizeof(char *));
  if (!files) {
    signal(SIGPIPE, old_sigpipe);
    return fields_io_err("out of memory");
  }

  int rc = 2; // default to error unless set

  // === ANCHOR:ARGV-PARSE-BEGIN ===
  for (WORD_LIST *w = list; w; w = w->next) {
    const char *tok = w->word->word;
    if (!tok) tok = "";

    if (!spec) {
      if (!end_opts && strcmp(tok, "--help") == 0) {
        rc = fields_help();
        goto out;
      }
      if (!end_opts && strcmp(tok, "--") == 0) {
        end_opts = true;
        continue;
      }

      if (!end_opts && strncmp(tok, "-d", 2) == 0 && tok[1] == 'd') {
        const char *darg = NULL;
        if (tok[2] != '\0') {
          // -dX form
          darg = tok + 2;
        } else {
          // -d X form; take next WORD_LIST element as argument.
          if (!w->next) {
            rc = fields_usage_err("missing DELIM for -d");
            goto out;
          }
          w = w->next;
          darg = w->word->word;
          if (!darg) darg = "";
        }

        if (delim_mode) {
          rc = fields_usage_err("duplicate -d");
          goto out;
        }
        if (strlen(darg) != 1) {
          rc = fields_usage_err("DELIM must be exactly 1 byte");
          goto out;
        }

        delim_mode = true;
        delim = (uint8_t)darg[0];
        continue;
      }

      if (!end_opts && tok[0] == '-' && tok[1] != '\0' && strcmp(tok, "-") != 0) {
        rc = fields_usage_err("unknown option (use --help)");
        goto out;
      }

      spec = tok;
      continue;
    }

    if (!end_opts && strcmp(tok, "--") == 0) {
      end_opts = true;
      continue;
    }

    if (!end_opts && tok[0] == '-' && tok[1] != '\0' && strcmp(tok, "-") != 0) {
      rc = fields_usage_err("unknown option (use --help)");
      goto out;
    }

    if (fcnt == fcap) {
      size_t ncap = fcap * 2;
      char **nf = (char **)realloc(files, ncap * sizeof(char *));
      if (!nf) {
        rc = fields_io_err("out of memory");
        goto out;
      }
      files = nf;
      fcap = ncap;
    }
    files[fcnt++] = (char *)tok;
  }
  // === ANCHOR:ARGV-PARSE-END ===

  if (!spec) {
    rc = fields_usage_err("missing SPEC");
    goto out;
  }

  rc = fields_main(spec, delim_mode, delim, files, fcnt);

out:
  // === ANCHOR:CLEANUP-BEGIN ===
  free(files);
  signal(SIGPIPE, old_sigpipe);
  return rc;
  // === ANCHOR:CLEANUP-END ===
}

__attribute__((visibility("default")))
struct builtin fields_struct = {
  .name = "fields",
  .function = fields_builtin,
  .flags = BUILTIN_ENABLED,
  .long_doc = fields_doc,
  .short_doc = (char *)"fields [-d DELIM] SPEC [--] [FILE...]",
  .handle = 0,
};