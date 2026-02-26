#include "diamondcore.h"

#include <stdio.h>

void dc_print_usage_take(FILE *out) {
  if (!out) out = stdout;
  fputs("usage: take N [S] [--] [FILE...]\n", out);
  fputs("       take --help\n", out);
}