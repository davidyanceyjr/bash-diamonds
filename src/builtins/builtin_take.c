// builtin_take.c - `take` loadable builtin

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
static const char *take_shortdoc = "take N [S] [--] [FILE...]";

static char *take_doc[] = {
  "Emit a forward-only slice of input lines (take N [S]).",
  (char *)0,
};

static int take_usage_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "take: %s\n", msg);
  else dc_print_usage_take(stderr);
  return 2;
}

static int take_io_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "take: %s\n", msg);
  else fprintf(stderr, "take: I/O error\n");
  return 2;
}

static int take_help(void) {
  dc_print_usage_take(stdout);
  return 0;
}

static int take_main(uint64_t n, uint64_t s, char *const *files, size_t file_count) {
  dc_error_t err;

  dc_line_reader_t *lr = dc_lr_open(files, file_count, &err);
  if (!lr) {
    return take_io_err(err.msg[0] ? err.msg : "cannot open input");
  }

  uint64_t line_no = 0;
  uint64_t emitted = 0;

  for (;;) {
    dc_line_view_t v;
    bool ok = dc_lr_next(lr, &v, &err);
    if (!ok) {
      if (err.code != DC_ERR_NONE) {
        dc_lr_close(lr);
        return take_io_err(err.msg[0] ? err.msg : "read error");
      }
      break; // EOF
    }

    line_no++;

    if (line_no <= s) continue;
    if (emitted >= n) break;

    if (n > 0) {
      if (v.len > 0) {
        size_t wrote = fwrite(v.ptr, 1, v.len, stdout);
        if (wrote != v.len) {
          dc_lr_close(lr);
          return take_io_err("write error");
        }
      }
      emitted++;
      if (emitted >= n) break;
    }
  }

  dc_lr_close(lr);
  return (emitted > 0) ? 0 : 1;
}

// Parsing rules:
// - Only --help and -- are recognized.
// - Any other -x token is an error unless after --, or token is exactly '-'.
// - N is required (first positional).
// - S is optional (second positional).
// - Remaining tokens are FILE...
__attribute__((visibility("default")))
int take_builtin(WORD_LIST *list) {
  // === ANCHOR:SIGPIPE-BEGIN ===
  // Ignore SIGPIPE so closed-pipe writes surface as stdio errors (EPIPE) and we return 2.
  void (*old_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
  // === ANCHOR:SIGPIPE-END ===

  bool end_opts = false;
  bool have_n = false;
  bool have_s = false;
  uint64_t n = 0;
  uint64_t s = 0;

  size_t fcap = 8;
  size_t fcnt = 0;
  char **files = (char **)calloc(fcap, sizeof(char *));
  if (!files) {
    signal(SIGPIPE, old_sigpipe);
    return take_io_err("out of memory");
  }

  int rc = 2;

  // === ANCHOR:ARGV-PARSE-BEGIN ===
  for (WORD_LIST *w = list; w; w = w->next) {
    const char *tok = w->word->word;
    if (!tok) tok = "";

    if (!end_opts && !have_n && strcmp(tok, "--help") == 0) {
      rc = take_help();
      goto out;
    }

    if (!end_opts && strcmp(tok, "--") == 0) {
      end_opts = true;
      continue;
    }

    if (!end_opts && tok[0] == '-' && tok[1] != '\0' && strcmp(tok, "-") != 0) {
      rc = take_usage_err("unknown option (use --help)");
      goto out;
    }

    if (!have_n) {
      dc_error_t err;
      if (!dc_parse_u64_dec_strict(tok, &n, "N", &err)) {
        rc = take_usage_err(err.msg[0] ? err.msg : "invalid N");
        goto out;
      }
      have_n = true;
      continue;
    }

    if (!have_s) {
      dc_error_t err;
      if (!dc_parse_u64_dec_strict(tok, &s, "S", &err)) {
        rc = take_usage_err(err.msg[0] ? err.msg : "invalid S");
        goto out;
      }
      have_s = true;
      continue;
    }

    if (fcnt == fcap) {
      size_t ncap = fcap * 2;
      char **nf = (char **)realloc(files, ncap * sizeof(char *));
      if (!nf) {
        rc = take_io_err("out of memory");
        goto out;
      }
      files = nf;
      fcap = ncap;
    }
    files[fcnt++] = (char *)tok;
  }
  // === ANCHOR:ARGV-PARSE-END ===

  if (!have_n) {
    rc = take_usage_err("missing N");
    goto out;
  }

  rc = take_main(n, have_s ? s : 0, files, fcnt);

out:
  // === ANCHOR:CLEANUP-BEGIN ===
  free(files);
  signal(SIGPIPE, old_sigpipe);
  return rc;
  // === ANCHOR:CLEANUP-END ===
}

__attribute__((visibility("default")))
struct builtin take_struct = {
  .name = "take",
  .function = take_builtin,
  .flags = BUILTIN_ENABLED,
  .long_doc = take_doc,
  .short_doc = (char *)"take N [S] [--] [FILE...]",
  .handle = 0,
};