#include "diamondcore.h"

#include <stdio.h>

void dc_print_help_count(FILE *out) {
  if (!out) out = stdout;

  fputs("usage: count [--] [FILE...]\n", out);
  fputs("       count --help\n", out);
  fputs("\n", out);
  fputs("Count input lines.\n", out);
  fputs("\n", out);
  fputs("Options:\n", out);
  fputs("  --help   print this help and exit 0\n", out);
  fputs("  --       end option parsing\n", out);
  fputs("\n", out);
  fputs("Arguments:\n", out);
  fputs("  FILE...  input files ('-' means stdin)\n", out);
  fputs("\n", out);
  fputs("Input:\n", out);
  fputs("  - Input is processed as a sequence of lines.\n", out);
  fputs("  - A line ends in '\\n', or is the final unterminated line at EOF.\n", out);
  fputs("  - The final unterminated line counts as a line.\n", out);
  fputs("\n", out);
  fputs("Output:\n", out);
  fputs("  - Prints the line count as base-10 unsigned decimal followed by '\\n'.\n", out);
  fputs("\n", out);
  fputs("Exit codes:\n", out);
  fputs("  0  success (count printed)\n", out);
  fputs("  2  usage/runtime error\n", out);
  fputs("\n", out);
  fputs("SIGPIPE:\n", out);
  fputs("  SIGPIPE is ignored; any stdout write failure exits 2.\n", out);
  fputs("\n", out);
  fputs("Examples:\n", out);
  fputs("  printf \"a\\nb\\n\" | count\n", out);
  fputs("  printf \"a\\nb\" | count\n", out);
  fputs("  count a.txt b.txt\n", out);
  fputs("  count a.txt - b.txt\n", out);
}