#include "diamondcore.h"

#include <stdio.h>

void dc_print_usage_count(FILE *out) {
  fprintf(out, "usage: count [--] [FILE...]\n"
               "       count --help\n");
}