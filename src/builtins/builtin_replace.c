#include "dc_regex.h"
#include "diamondcore.h"

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "shell.h"

static char *replace_doc[] = {
    "Perform per-line substitution and emit only modified lines.",
    (char *)0,
};

static int usage_err(const char *msg) {
  if (msg && *msg)
    fprintf(stderr, "replace: %s\n", msg);
  dc_print_usage_replace(stderr);
  return 2;
}

static bool write_bytes(const uint8_t *p, size_t n) {
  if (n == 0)
    return true;
  size_t w = fwrite(p, 1, n, stdout);
  return (w == n) && !ferror(stdout);
}

static bool bytes_eq(const uint8_t *a, const uint8_t *b, size_t n) {
  if (n == 0)
    return true;
  return memcmp(a, b, n) == 0;
}

static bool literal_find_next(const uint8_t *hay, size_t hay_len,
                              const uint8_t *needle, size_t needle_len,
                              size_t start_at, size_t *out_start,
                              size_t *out_end) {
  if (!hay || !needle)
    return false;
  if (needle_len == 0)
    return false;
  if (start_at > hay_len)
    return false;
  if (needle_len > hay_len)
    return false;

  size_t last = hay_len - needle_len;
  for (size_t i = start_at; i <= last; i++) {
    if (bytes_eq(hay + i, needle, needle_len)) {
      if (out_start)
        *out_start = i;
      if (out_end)
        *out_end = i + needle_len;
      return true;
    }
  }
  return false;
}

static int process_record_literal(const uint8_t *content, size_t content_len,
                                  const uint8_t *pat, size_t pat_len,
                                  const uint8_t *rep, size_t rep_len,
                                  bool has_delim) {
  size_t s = 0, e = 0;
  if (!literal_find_next(content, content_len, pat, pat_len, 0, &s, &e))
    return 1;

  size_t last = 0;
  size_t cursor = 0;

  for (;;) {
    if (!literal_find_next(content, content_len, pat, pat_len, cursor, &s, &e))
      break;

    if (s > last) {
      if (!write_bytes(content + last, s - last))
        return 2;
    }
    if (rep_len > 0) {
      if (!write_bytes(rep, rep_len))
        return 2;
    }

    last = e;
    cursor = e; /* non-overlapping */
  }

  if (last < content_len) {
    if (!write_bytes(content + last, content_len - last))
      return 2;
  }

  if (has_delim) {
    uint8_t nl = 0x0A;
    if (!write_bytes(&nl, 1))
      return 2;
  }

  return 0;
}

static int process_record_regex(const uint8_t *content, size_t content_len,
                                const dc_regex_t *re, const uint8_t *rep,
                                size_t rep_len, bool has_delim) {
  size_t s = 0, e = 0;
  bool limit = false;

  if (!dc_regex_find_next(re, content, content_len, 0, &s, &e, &limit)) {
    if (limit)
      return 3;
    return 1;
  }

  size_t last = 0;
  size_t cursor = 0;

  for (;;) {
    limit = false;
    if (!dc_regex_find_next(re, content, content_len, cursor, &s, &e, &limit)) {
      if (limit)
        return 3;
      break;
    }

    if (s > last) {
      if (!write_bytes(content + last, s - last))
        return 2;
    }
    if (rep_len > 0) {
      if (!write_bytes(rep, rep_len))
        return 2;
    }

    last = e;

    /* Zero-length match must advance by >= 1 byte */
    if (e > cursor)
      cursor = e;
    else
      cursor = (cursor < content_len) ? (cursor + 1) : (content_len + 1);

    if (cursor > content_len)
      break;
  }

  if (last < content_len) {
    if (!write_bytes(content + last, content_len - last))
      return 2;
  }

  if (has_delim) {
    uint8_t nl = 0x0A;
    if (!write_bytes(&nl, 1))
      return 2;
  }

  return 0;
}

static int replace_builtin(WORD_LIST *list) {
  void (*old_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);

  bool end_opts = false;
  bool literal = false;
  bool saw_literal = false;

  const char *pattern_s = NULL;
  const char *replacement_s = NULL;

  /* collect FILE... (optional) */
  size_t file_cap = 8;
  size_t file_count = 0;
  char **files = (char **)calloc(file_cap, sizeof(char *));
  if (!files) {
    signal(SIGPIPE, old_sigpipe);
    return 2;
  }

  for (WORD_LIST *w = list; w; w = w->next) {
    const char *arg = w->word->word;
    if (!arg)
      continue;

    /* --help only recognized during option parsing, before PATTERN */
    if (!end_opts && pattern_s == NULL && strcmp(arg, "--help") == 0) {
      dc_print_help_replace(stdout);
      free(files);
      signal(SIGPIPE, old_sigpipe);
      return 0;
    }

    if (!end_opts && strcmp(arg, "--") == 0) {
      end_opts = true;
      continue;
    }

    if (!end_opts && strcmp(arg, "--literal") == 0) {
      if (pattern_s != NULL) {
        free(files);
        signal(SIGPIPE, old_sigpipe);
        return usage_err("--literal must precede PATTERN");
      }
      if (saw_literal) {
        free(files);
        signal(SIGPIPE, old_sigpipe);
        return usage_err("duplicate --literal");
      }
      saw_literal = true;
      literal = true;
      continue;
    }

    /* Strict option parsing: any '-' token before '--' and before PATTERN is an
     * option token. Since replace has positional-primary PATTERN, unknown
     * option tokens are usage errors. This intentionally includes single "-"
     * (pattern beginning with '-' requires '--').
     */
    if (!end_opts && pattern_s == NULL && arg[0] == '-') {
      free(files);
      signal(SIGPIPE, old_sigpipe);
      return usage_err("unknown option");
    }

    if (pattern_s == NULL) {
      pattern_s = arg;
      continue;
    }
    if (replacement_s == NULL) {
      replacement_s = arg;
      continue;
    }

    if (file_count == file_cap) {
      size_t new_cap = file_cap * 2;
      char **nf = (char **)realloc(files, new_cap * sizeof(char *));
      if (!nf) {
        free(files);
        signal(SIGPIPE, old_sigpipe);
        return 2;
      }
      files = nf;
      memset(files + file_cap, 0, (new_cap - file_cap) * sizeof(char *));
      file_cap = new_cap;
    }
    files[file_count++] = (char *)arg;
  }

  if (!pattern_s || !replacement_s) {
    free(files);
    signal(SIGPIPE, old_sigpipe);
    return usage_err("missing PATTERN or REPLACEMENT");
  }

  size_t pat_len = strlen(pattern_s);
  if (pat_len == 0) {
    free(files);
    signal(SIGPIPE, old_sigpipe);
    return usage_err("empty PATTERN");
  }

  const uint8_t *pat = (const uint8_t *)pattern_s;
  const uint8_t *rep = (const uint8_t *)replacement_s;
  size_t rep_len = strlen(replacement_s);

  dc_regex_t *re = NULL;
  if (!literal) {
    char errbuf[256];
    errbuf[0] = '\0';
    if (!dc_regex_compile(&re, pattern_s, errbuf)) {
      if (errbuf[0]) {
        if (strncmp(errbuf, "match:", 6) == 0)
          fprintf(stderr, "replace:%s\n", errbuf + 6);
        else
          fprintf(stderr, "replace: %s\n", errbuf);
      } else {
        fprintf(stderr, "replace: regex compile error\n");
      }
      free(files);
      signal(SIGPIPE, old_sigpipe);
      return 2;
    }
  }

  dc_error_t err;
  dc_err_init(&err);

  dc_line_reader_t *lr = dc_lr_open(files, file_count, &err);
  if (!lr) {
    if (re)
      dc_regex_free(re);
    free(files);
    signal(SIGPIPE, old_sigpipe);
    fprintf(stderr, "replace: %s\n",
            err.msg[0] ? err.msg : "cannot open input");
    return 2;
  }

  bool emitted_any = false;

  for (;;) {
    dc_line_view_t v;
    dc_err_init(&err);
    bool ok = dc_lr_next(lr, &v, &err);
    if (!ok) {
      if (err.code != DC_ERR_NONE) {
        dc_lr_close(lr);
        if (re)
          dc_regex_free(re);
        free(files);
        signal(SIGPIPE, old_sigpipe);
        fprintf(stderr, "replace: %s\n", err.msg[0] ? err.msg : "read error");
        return 2;
      }
      break;
    }

    size_t content_len = v.len;
    if (v.ends_with_nl && content_len > 0)
      content_len--;

    int rc;
    if (literal) {
      rc = process_record_literal(v.ptr, content_len, pat, pat_len, rep,
                                  rep_len, v.ends_with_nl);
    } else {
      rc = process_record_regex(v.ptr, content_len, re, rep, rep_len,
                                v.ends_with_nl);
      if (rc == 3) {
        fprintf(stderr, "replace: regex execution limit exceeded\n");
        dc_lr_close(lr);
        dc_regex_free(re);
        free(files);
        signal(SIGPIPE, old_sigpipe);
        return 2;
      }
    }

    if (rc == 0) {
      emitted_any = true;
    } else if (rc == 2) {
      dc_lr_close(lr);
      if (re)
        dc_regex_free(re);
      free(files);
      signal(SIGPIPE, old_sigpipe);
      return 2;
    }
  }

  dc_lr_close(lr);
  if (re)
    dc_regex_free(re);
  free(files);

  if (emitted_any) {
    if (fflush(stdout) != 0 || ferror(stdout)) {
      signal(SIGPIPE, old_sigpipe);
      return 2;
    }
  }

  signal(SIGPIPE, old_sigpipe);
  return emitted_any ? 0 : 1;
}

__attribute__((visibility("default"))) struct builtin replace_struct = {
    .name = "replace",
    .function = replace_builtin,
    .flags = BUILTIN_ENABLED,
    .long_doc = replace_doc,
    .short_doc =
        (char *)"replace [--literal] PATTERN REPLACEMENT [--] [FILE...]",
    .handle = 0,
};
