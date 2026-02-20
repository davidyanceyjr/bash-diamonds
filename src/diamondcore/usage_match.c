#include "diamondcore.h"

#include <stdio.h>

void dc_print_usage_match(FILE *out) {
  if (!out) out = stdout;
  fputs("usage: match PATTERN [--] [FILE...]\n", out);
  fputs("       match --help\n", out);
}
