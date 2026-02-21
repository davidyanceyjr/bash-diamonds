// builtin_trim.c - `trim` loadable builtin

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
static const char *trim_shortdoc = "trim [--] [FILE...]";

static char *trim_doc[] = {
  "Remove leading and trailing ASCII whitespace from each input line.",
  (char *)0,
};

static int trim_usage_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "trim: %s\n", msg);
  else dc_print_usage_trim(stderr);
  return 2;
}

static int trim_io_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "trim: %s\n", msg);
  else fprintf(stderr, "trim: I/O error\n");
  return 2;
}

static int trim_help(void) {
  dc_print_usage_trim(stdout);
  return 0;
}

static inline bool is_trim_ws(uint8_t c) {
  // ASCII whitespace to trim (newline is structural and handled separately)
  return (c == (uint8_t)' ' || c == (uint8_t)'\t' || c == (uint8_t)'\r' ||
          c == (uint8_t)'\v' || c == (uint8_t)'\f');
}

static int trim_main(char *const *files, size_t file_count) {
  dc_error_t err;
  dc_line_reader_t *lr = dc_lr_open(files, file_count, &err);
  if (!lr) {
    return trim_io_err(err.msg[0] ? err.msg : "cannot open input");
  }

  bool emitted_any = false;

  for (;;) {
    dc_line_view_t v;
    bool ok = dc_lr_next(lr, &v, &err);
    if (!ok) {
      if (err.code != DC_ERR_NONE) {
        dc_lr_close(lr);
        return trim_io_err(err.msg[0] ? err.msg : "read error");
      }
      break; // EOF
    }

    // Exclude trailing '\n' from trim region; newline is structural.
    size_t content_len = v.len;
    if (v.ends_with_nl && content_len > 0) content_len -= 1;

    size_t start = 0;
    while (start < content_len && is_trim_ws(v.ptr[start])) start++;

    size_t end = content_len;
    while (end > start && is_trim_ws(v.ptr[end - 1])) end--;

    size_t out_len = end - start;
    if (out_len == 0) continue; // emit nothing for this line

    size_t n = fwrite(v.ptr + start, 1, out_len, stdout);
    if (n != out_len) {
      dc_lr_close(lr);
      return trim_io_err("write error");
    }

    if (v.ends_with_nl) {
      if (fputc('\n', stdout) == EOF) {
        dc_lr_close(lr);
        return trim_io_err("write error");
      }
    }

    emitted_any = true;
  }

  dc_lr_close(lr);
  return emitted_any ? 0 : 1;
}

// Parsing rules:
// - Only --help is recognized.
// - Any other -x token is an error unless after --, or token is exactly '-'.
__attribute__((visibility("default")))
int trim_builtin(WORD_LIST *list) {
  // === ANCHOR:SIGPIPE-BEGIN ===
  // Ignore SIGPIPE so closed-pipe writes surface as stdio errors (EPIPE) and we return 2.
  void (*old_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
  // === ANCHOR:SIGPIPE-END ===

  bool end_opts = false;

  size_t fcap = 8;
  size_t fcnt = 0;
  char **files = (char **)calloc(fcap, sizeof(char *));
  if (!files) {
    signal(SIGPIPE, old_sigpipe);
    return trim_io_err("out of memory");
  }

  int rc = 2; // default error unless set

  for (WORD_LIST *w = list; w; w = w->next) {
    const char *tok = w->word->word;
    if (!tok) tok = "";

    if (!end_opts && strcmp(tok, "--help") == 0) {
      rc = trim_help();
      goto out;
    }
    if (!end_opts && strcmp(tok, "--") == 0) {
      end_opts = true;
      continue;
    }

    if (!end_opts && tok[0] == '-' && tok[1] != '\0' && strcmp(tok, "-") != 0) {
      rc = trim_usage_err("unknown option (use --help)");
      goto out;
    }

    if (fcnt == fcap) {
      size_t ncap = fcap * 2;
      char **nf = (char **)realloc(files, ncap * sizeof(char *));
      if (!nf) {
        rc = trim_io_err("out of memory");
        goto out;
      }
      files = nf;
      fcap = ncap;
    }
    files[fcnt++] = (char *)tok;
  }

  rc = trim_main(files, fcnt);

out:
  free(files);
  signal(SIGPIPE, old_sigpipe);
  return rc;
}

__attribute__((visibility("default")))
struct builtin trim_struct = {
  .name = "trim",
  .function = trim_builtin,
  .flags = BUILTIN_ENABLED,
  .long_doc = trim_doc,
  .short_doc = (char *)"trim [--] [FILE...]",
  .handle = 0,
};
