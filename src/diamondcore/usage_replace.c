/* src/diamondcore/usage_replace.c */
#include "diamondcore.h"

#include <stdio.h>

void dc_print_usage_replace(FILE *out) {
  if (!out)
    out = stdout;
  fputs("usage: replace [--literal] PATTERN REPLACEMENT [--] [FILE...]\n", out);
  fputs("       replace --help\n", out);
}