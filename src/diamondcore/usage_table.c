// usage_table.c - usage printer for `table`

#include "diamondcore.h"

#include <stdio.h>

void dc_print_usage_table(FILE *out) {
  if (!out) out = stdout;
  fputs("usage: table [--] [FILE...]\n", out);
  fputs("       table --help\n", out);
}