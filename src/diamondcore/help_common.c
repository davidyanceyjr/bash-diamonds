// help_common.c - shared help fragments for Diamond builtins

#include "diamondcore.h"

#include <stdio.h>

void dc_print_help_common_exit_codes(FILE *out) {
  if (!out) out = stdout;
  fputs("Exit codes:\n", out);
  fputs("  0  success with output\n", out);
  fputs("  1  valid execution, no result\n", out);
  fputs("  2  usage error or runtime error\n", out);
}

void dc_print_help_common_files(FILE *out) {
  if (!out) out = stdout;
  fputs("Input:\n", out);
  fputs("  - Reads FILE... left-to-right; if no FILE, reads stdin.\n", out);
  fputs("  - A filename of '-' means stdin at that position.\n", out);
  fputs("  - Inputs are concatenated logically.\n", out);
}

void dc_print_help_common_sigpipe(FILE *out) {
  if (!out) out = stdout;
  fputs("Notes:\n", out);
  fputs("  - SIGPIPE is ignored internally; a closed stdout is a controlled error (exit 2).\n", out);
}

void dc_print_help_common_range_spec(FILE *out, const char *what, const char *scope) {
  if (!out) out = stdout;
  if (!what) what = "SPEC";
  if (!scope) scope = "";

  fprintf(out, "%s:\n", what);
  if (scope[0]) {
    fprintf(out, "  %s\n", scope);
  }
  fputs("  Grammar (1-based):\n", out);
  fputs("    N        single index\n", out);
  fputs("    N,M      list\n", out);
  fputs("    A..B     closed range\n", out);
  fputs("    ..B      open start\n", out);
  fputs("    A..      open end\n", out);
  fputs("  Whitespace is allowed around ',' and '..'.\n", out);
  fputs("  Normalized: sorted, deduped, merged; reversed ranges and leading zeros are errors.\n", out);
}