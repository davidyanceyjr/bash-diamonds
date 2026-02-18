// builtin_lines.c - `lines` loadable builtin

#include "diamondcore.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>  // ANCHOR:SIGPIPE-INCLUDE

// Bash loadable builtin headers (provided by bash source / headers)
#include "config.h"
#include "builtins.h"
#include "shell.h"

__attribute__((unused))
static const char *lines_shortdoc = "lines SPEC [--] [FILE...]";

static char *lines_doc[] = {
  "Select and emit specific 1-based input lines by numeric index or range.",
  (char *)0,
};

static int lines_usage_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "lines: %s\n", msg);
  else dc_print_usage_lines(stderr);
  return 2;
}

static int lines_io_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "lines: %s\n", msg);
  else fprintf(stderr, "lines: I/O error\n");
  return 2;
}

static int lines_help(void) {
  dc_print_usage_lines(stdout);
  return 0;
}

static int lines_main(const char *spec, char *const *files, size_t file_count) {
  dc_error_t err;
  dc_sel_t *sel = dc_sel_parse_and_normalize(spec, &err);
  if (!sel) {
    return lines_usage_err(err.msg[0] ? err.msg : "invalid SPEC");
  }

  bool has_max = false;
  uint64_t max_finite = dc_sel_max_finite(sel, &has_max);

  dc_line_reader_t *lr = dc_lr_open(files, file_count, &err);
  if (!lr) {
    dc_sel_free(sel);
    return lines_io_err(err.msg[0] ? err.msg : "cannot open input");
  }

  uint64_t line_no = 0;
  bool emitted = false;

  for (;;) {
    dc_line_view_t v;
    bool ok = dc_lr_next(lr, &v, &err);
    if (!ok) {
      if (err.code != DC_ERR_NONE) {
        dc_lr_close(lr);
        dc_sel_free(sel);
        return lines_io_err(err.msg[0] ? err.msg : "read error");
      }
      break; // EOF
    }

    line_no++;

    if (dc_sel_wants(sel, line_no)) {
      if (v.len > 0) {
        size_t n = fwrite(v.ptr, 1, v.len, stdout);
        if (n != v.len) {
          dc_lr_close(lr);
          dc_sel_free(sel);
          return lines_io_err("write error");
        }
      }
      emitted = true;
    }

    if (has_max && line_no >= max_finite) {
      // Proven no future lines needed.
      break;
    }
  }

  dc_lr_close(lr);
  dc_sel_free(sel);
  return emitted ? 0 : 1;
}

// Parsing rules:
// - Only --help is recognized.
// - Any other -x token is an error unless after --, or token is exactly '-'.
// - SPEC is required and is the first non-option token.
__attribute__((visibility("default")))
int lines_builtin(WORD_LIST *list) {
  // === ANCHOR:SIGPIPE-BEGIN ===
  // Ignore SIGPIPE so closed-pipe writes surface as stdio errors (EPIPE) and we return 2.
  void (*old_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
  // === ANCHOR:SIGPIPE-END ===

  bool end_opts = false;
  const char *spec = NULL;

  size_t fcap = 8;
  size_t fcnt = 0;
  char **files = (char **)calloc(fcap, sizeof(char *));
  if (!files) {
    signal(SIGPIPE, old_sigpipe);
    return lines_io_err("out of memory");
  }

  int rc = 2; // default error unless set

  // === ANCHOR:ARGV-PARSE-BEGIN ===
  for (WORD_LIST *w = list; w; w = w->next) {
    const char *tok = w->word->word;
    if (!tok) tok = "";

    if (!spec) {
      if (!end_opts && strcmp(tok, "--help") == 0) {
        rc = lines_help();
        goto out;
      }
      if (!end_opts && strcmp(tok, "--") == 0) {
        end_opts = true;
        continue;
      }

      if (!end_opts && tok[0] == '-' && tok[1] != '\0' && strcmp(tok, "-") != 0) {
        rc = lines_usage_err("unknown option (use --help)");
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
      rc = lines_usage_err("unknown option (use --help)");
      goto out;
    }

    if (fcnt == fcap) {
      size_t ncap = fcap * 2;
      char **nf = (char **)realloc(files, ncap * sizeof(char *));
      if (!nf) {
        rc = lines_io_err("out of memory");
        goto out;
      }
      files = nf;
      fcap = ncap;
    }
    files[fcnt++] = (char *)tok;
  }
  // === ANCHOR:ARGV-PARSE-END ===

  if (!spec) {
    rc = lines_usage_err("missing SPEC");
    goto out;
  }

  rc = lines_main(spec, files, fcnt);

out:
  // === ANCHOR:CLEANUP-BEGIN ===
  free(files);
  signal(SIGPIPE, old_sigpipe);
  return rc;
  // === ANCHOR:CLEANUP-END ===
}

__attribute__((visibility("default")))
struct builtin lines_struct = {
  .name = "lines",
  .function = lines_builtin,
  .flags = BUILTIN_ENABLED,
  .long_doc = lines_doc,
  .short_doc = (char *)"lines SPEC [--] [FILE...]",
  .handle = 0,
};
