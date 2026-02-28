#include "diamondcore.h"

#include <stdio.h>

void dc_print_usage_filter(FILE *out) {
  if (!out) out = stdout;
  fputs("usage: filter EXPR [--] [FILE...]\n", out);
  fputs("       filter --help\n", out);
}