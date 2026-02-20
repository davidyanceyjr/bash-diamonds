// builtin_match.c - `match` loadable builtin

#include "diamondcore.h"
#include "dc_regex.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>  // ANCHOR:SIGPIPE-INCLUDE

#include "config.h"
#include "builtins.h"
#include "shell.h"

__attribute__((unused))
static const char *match_shortdoc = "match PATTERN [--] [FILE...]";

static char *match_doc[] = {
  "Filter input lines by a deterministic, constrained regex.",
  (char *)0,
};

static int match_usage_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "match: %s\n", msg);
  else dc_print_usage_match(stderr);
  return 2;
}

static int match_io_err(const char *msg) {
  if (msg && *msg) fprintf(stderr, "match: %s\n", msg);
  else fprintf(stderr, "match: I/O error\n");
  return 2;
}

static int match_help(void) {
  dc_print_usage_match(stdout);
  return 0;
}

static int match_main(const char *pattern, char *const *files, size_t file_count) {
  char errbuf[256];
  dc_regex_t *re = NULL;

  if (!dc_regex_compile(&re, pattern, errbuf)) {
    if (errbuf[0]) fprintf(stderr, "%s\n", errbuf);
    else fprintf(stderr, "match: pattern compile error\n");
    return 2;
  }

  dc_error_t err;
  dc_line_reader_t *lr = dc_lr_open(files, file_count, &err);
  if (!lr) {
    dc_regex_free(re);
    return match_io_err(err.msg[0] ? err.msg : "cannot open input");
  }

  bool emitted = false;

  for (;;) {
    dc_line_view_t v;
    bool ok = dc_lr_next(lr, &v, &err);
    if (!ok) {
      if (err.code != DC_ERR_NONE) {
        dc_lr_close(lr);
        dc_regex_free(re);
        return match_io_err(err.msg[0] ? err.msg : "read error");
      }
      break; /* EOF */
    }

    size_t subj_len = v.len;
    if (v.ends_with_nl && subj_len > 0) subj_len--;

    bool exec_limit = false;
    bool matched = dc_regex_match_line(re, v.ptr, subj_len, &exec_limit);
    if (exec_limit) {
      fprintf(stderr, "match: regex execution limit exceeded\n");
      dc_lr_close(lr);
      dc_regex_free(re);
      return 2;
    }

    if (matched) {
      if (v.len > 0) {
        size_t n = fwrite(v.ptr, 1, v.len, stdout);
        if (n != v.len) {
          dc_lr_close(lr);
          dc_regex_free(re);
          return match_io_err("write error");
        }
      }
      emitted = true;
    }
  }

  dc_lr_close(lr);
  dc_regex_free(re);
  return emitted ? 0 : 1;
}

/*
Parsing rules (same style as lines):
- Only --help is recognized.
- Any other -x token is an error unless after --, or token is exactly '-'.
- PATTERN is required and is the first non-option token.
*/
__attribute__((visibility("default")))
int match_builtin(WORD_LIST *list) {
  // === ANCHOR:SIGPIPE-BEGIN ===
  void (*old_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
  // === ANCHOR:SIGPIPE-END ===

  bool end_opts = false;
  const char *pattern = NULL;

  size_t fcap = 8;
  size_t fcnt = 0;
  char **files = (char **)calloc(fcap, sizeof(char *));
  if (!files) {
    signal(SIGPIPE, old_sigpipe);
    return match_io_err("out of memory");
  }

  int rc = 2;

  for (WORD_LIST *w = list; w; w = w->next) {
    const char *tok = w->word->word;
    if (!tok) tok = "";

    if (!pattern) {
      if (!end_opts && strcmp(tok, "--help") == 0) { rc = match_help(); goto out; }
      if (!end_opts && strcmp(tok, "--") == 0) { end_opts = true; continue; }

      if (!end_opts && tok[0] == '-' && tok[1] != '\0' && strcmp(tok, "-") != 0) {
        rc = match_usage_err("unknown option (use --help)");
        goto out;
      }

      pattern = tok;
      continue;
    }

    if (!end_opts && strcmp(tok, "--") == 0) { end_opts = true; continue; }

    if (!end_opts && tok[0] == '-' && tok[1] != '\0' && strcmp(tok, "-") != 0) {
      rc = match_usage_err("unknown option (use --help)");
      goto out;
    }

    if (fcnt == fcap) {
      size_t ncap = fcap * 2;
      char **nf = (char **)realloc(files, ncap * sizeof(char *));
      if (!nf) { rc = match_io_err("out of memory"); goto out; }
      files = nf;
      fcap = ncap;
    }
    files[fcnt++] = (char *)tok;
  }

  if (!pattern) { rc = match_usage_err("missing PATTERN"); goto out; }

  rc = match_main(pattern, files, fcnt);

out:
  free(files);
  signal(SIGPIPE, old_sigpipe);
  return rc;
}

__attribute__((visibility("default")))
struct builtin match_struct = {
  .name = "match",
  .function = match_builtin,
  .flags = BUILTIN_ENABLED,
  .long_doc = match_doc,
  .short_doc = (char *)"match PATTERN [--] [FILE...]",
  .handle = 0,
};
