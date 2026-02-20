// io.c - streaming line reader across stdin and/or files

#include "diamondcore.h"

#include <sys/types.h>  /* ssize_t */
#include <stdio.h>      /* getline */
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct dc_line_reader {
  char **files;
  size_t file_count;
  size_t idx;
  FILE *fp;
  bool fp_is_stdin;
  char *buf;
  size_t buf_cap;
};

static bool open_next(dc_line_reader_t *lr, dc_error_t *err) {
  if (!lr) return false;
  if (lr->fp && !lr->fp_is_stdin) {
    fclose(lr->fp);
  }
  lr->fp = NULL;
  lr->fp_is_stdin = false;

  if (lr->idx >= lr->file_count) return false;

  const char *name = lr->files[lr->idx++];
  if (strcmp(name, "-") == 0) {
    lr->fp = stdin;
    lr->fp_is_stdin = true;
    return true;
  }

  lr->fp = fopen(name, "rb");
  if (!lr->fp) {
    dc_err_set(err, DC_ERR_IO, "cannot open '%s': %s", name, strerror(errno));
    return false;
  }
  return true;
}

dc_line_reader_t *dc_lr_open(char *const *files, size_t file_count, dc_error_t *err) {
  dc_err_init(err);
  dc_line_reader_t *lr = (dc_line_reader_t *)calloc(1, sizeof(dc_line_reader_t));
  if (!lr) {
    dc_err_set(err, DC_ERR_NOMEM, "out of memory");
    return NULL;
  }

  if (file_count == 0) {
    // Default: stdin only.
    lr->files = (char **)calloc(1, sizeof(char *));
    if (!lr->files) {
      free(lr);
      dc_err_set(err, DC_ERR_NOMEM, "out of memory");
      return NULL;
    }
    lr->files[0] = "-";
    lr->file_count = 1;
  } else {
    lr->files = (char **)calloc(file_count, sizeof(char *));
    if (!lr->files) {
      free(lr);
      dc_err_set(err, DC_ERR_NOMEM, "out of memory");
      return NULL;
    }
    for (size_t i = 0; i < file_count; i++) lr->files[i] = files[i];
    lr->file_count = file_count;
  }

  lr->idx = 0;
  lr->fp = NULL;
  lr->buf = NULL;
  lr->buf_cap = 0;

  // Defer opening until first dc_lr_next.
  return lr;
}

bool dc_lr_next(dc_line_reader_t *lr, dc_line_view_t *out, dc_error_t *err) {
  dc_err_init(err);
  if (!lr || !out) {
    dc_err_set(err, DC_ERR_INTERNAL, "internal: null reader/out");
    return false;
  }

  for (;;) {
    if (!lr->fp) {
      if (!open_next(lr, err)) {
        // If open_next fails with err set => error; otherwise EOF.
        return false;
      }
    }

    errno = 0;
    ssize_t n = getline(&lr->buf, &lr->buf_cap, lr->fp);
    if (n >= 0) {
      out->ptr = (const uint8_t *)lr->buf;
      out->len = (size_t)n;
      out->ends_with_nl = (n > 0 && lr->buf[n - 1] == '\n');
      return true;
    }

    // n < 0
    if (feof(lr->fp)) {
      // Move to next source.
      if (lr->fp && !lr->fp_is_stdin) fclose(lr->fp);
      lr->fp = NULL;
      lr->fp_is_stdin = false;
      continue;
    }

    // Error
    dc_err_set(err, DC_ERR_IO, "read error: %s", strerror(errno ? errno : EIO));
    return false;
  }
}

void dc_lr_close(dc_line_reader_t *lr) {
  if (!lr) return;
  if (lr->fp && !lr->fp_is_stdin) fclose(lr->fp);
  free(lr->files);
  free(lr->buf);
  free(lr);
}
