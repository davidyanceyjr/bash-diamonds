#include "diamondcore.h"

#include <stdio.h>

void dc_print_usage_trim(FILE *out) {
  fprintf(out,
          "usage: trim [--] [FILE...]\n"
          "       trim --help\n"
          "\n"
          "Remove leading and trailing ASCII whitespace from each input line.\n");
}
