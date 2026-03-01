/*
 * usage_fields.c
 *
 * Usage printer for `fields`.
 * Keep non-empty under -Wpedantic -Werror.
 */

#include "diamondcore.h"

#include <stdio.h>

void dc_print_usage_fields(FILE *out) {
  if (!out) out = stdout;
  fputs("usage: fields SPEC [FILE...]\n", out);
  fputs("       fields [--tsv] [-d DELIM] SPEC [--] [FILE...]\n", out);
  fputs("       fields --help\n", out);
}