// usage_lines.c - usage printer for `lines`

#include "diamondcore.h"

#include <stdio.h>

void dc_print_usage_lines(FILE *out) {
  if (!out) out = stdout;
  fputs("usage: lines SPEC [--] [FILE...]\n", out);
  fputs("       lines --help\n", out);
}
