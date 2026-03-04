// builtin_count.c - `count` loadable builtin

#include "diamondcore.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// Bash loadable builtin headers
#include "builtins.h"
#include "shell.h"

__attribute__((unused))
static const char *count_shortdoc = "count [--] [FILE...]";

static char *count_doc[] = {
  "Count input lines.",
  (char *)0,
};

static int count_usage_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "count: %s\n", msg);
  else dc_print_usage_count(stderr);
  return 2;
}

static int count_io_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "count: %s\n", msg);
  else fprintf(stderr, "count: I/O error\n");
  return 2;
}

static int count_help(void) {
  dc_print_help_count(stdout);
  return 0;
}

static int count_main(char *const *files, size_t file_count) {
  dc_error_t err;
  dc_line_reader_t *lr = dc_lr_open(files, file_count, &err);
  if (!lr) {
    return count_io_err(err.msg[0] ? err.msg : "cannot open input");
  }

  uint64_t count = 0;

  for (;;) {
    dc_line_view_t v;
    bool ok = dc_lr_next(lr, &v, &err);
    if (!ok) {
      if (err.code != DC_ERR_NONE) {
        dc_lr_close(lr);
        return count_io_err(err.msg[0] ? err.msg : "read error");
      }
      break;
    }
    count++;
  }

  dc_lr_close(lr);

  if (printf("%" PRIu64 "\n", count) < 0) {
    return count_io_err("write error");
  }
  if (fflush(stdout) == EOF || ferror(stdout)) {
    return count_io_err("write error");
  }

  return 0;
}

__attribute__((visibility("default")))
int count_builtin(WORD_LIST *list) {
  // === ANCHOR:SIGPIPE-BEGIN ===
  void (*old_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
  // === ANCHOR:SIGPIPE-END ===

  bool end_opts = false;

  size_t fcap = 8;
  size_t fcnt = 0;
  char **files = (char **)calloc(fcap, sizeof(char *));
  if (!files) {
    signal(SIGPIPE, old_sigpipe);
    return count_io_err("out of memory");
  }

  int rc = 2;

  // === ANCHOR:ARGV-PARSE-BEGIN ===
  for (WORD_LIST *w = list; w; w = w->next) {
    const char *tok = w->word->word;
    if (!tok) tok = "";

    if (!end_opts && strcmp(tok, "--help") == 0) {
      rc = count_help();
      goto out;
    }

    if (!end_opts && strcmp(tok, "--") == 0) {
      end_opts = true;
      continue;
    }

    if (!end_opts && tok[0] == '-' && tok[1] != '\0' && strcmp(tok, "-") != 0) {
      rc = count_usage_err("unknown option (use --help)");
      goto out;
    }

    if (fcnt == fcap) {
      size_t ncap = fcap * 2;
      char **nf = (char **)realloc(files, ncap * sizeof(char *));
      if (!nf) {
        rc = count_io_err("out of memory");
        goto out;
      }
      files = nf;
      fcap = ncap;
    }
    files[fcnt++] = (char *)tok;
  }
  // === ANCHOR:ARGV-PARSE-END ===

  rc = count_main(files, fcnt);

out:
  // === ANCHOR:CLEANUP-BEGIN ===
  free(files);
  signal(SIGPIPE, old_sigpipe);
  return rc;
  // === ANCHOR:CLEANUP-END ===
}

__attribute__((visibility("default")))
struct builtin count_struct = {
  .name = "count",
  .function = count_builtin,
  .flags = BUILTIN_ENABLED,
  .long_doc = count_doc,
  .short_doc = (char *)"count [--] [FILE...]",
  .handle = 0,
};
