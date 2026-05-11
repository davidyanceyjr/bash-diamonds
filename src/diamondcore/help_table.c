// help_table.c - help printer for `table`

#include "diamondcore.h"

#include <stdio.h>

void dc_print_help_table(FILE *out) {
  if (!out)
    out = stdout;

  fputs("usage: table [--] [FILE...]\n", out);
  fputs("       table --help\n", out);
  fputs("\n", out);
  fputs("Format delimited text into aligned columns for human output.\n", out);
  fputs("\n", out);

  fputs("Options:\n", out);
  fputs("  --help   print this help and exit 0\n", out);
  fputs("  --       end option parsing\n", out);
  fputs("\n", out);

  fputs("Arguments:\n", out);
  fputs("  FILE...  input files (must be seekable regular files)\n", out);
  fputs("\n", out);

  fputs("Input:\n", out);
  fputs("  - Fields are split by runs of spaces and tabs (leading/trailing "
        "ignored).\n",
        out);
  fputs("  - Empty/whitespace-only lines are not emitted.\n", out);
  fputs("  - stdin (including '-') is not supported (non-seekable input).\n",
        out);
  fputs("\n", out);

  fputs("Output:\n", out);
  fputs("  - Columns are padded with spaces to align to the maximum width per "
        "column.\n",
        out);
  fputs("  - Minimum column separation: 1 space after column 1; 2 spaces after "
        "later columns.\n",
        out);
  fputs("  - No trailing spaces are written.\n", out);
  fputs("  - Newlines are preserved only when an input line is emitted.\n",
        out);
  fputs("\n", out);

  dc_print_help_common_exit_codes(out);
  fputs("\n", out);
  dc_print_help_common_sigpipe(out);
  fputs("\n", out);

  fputs("Examples:\n", out);
  fputs("  table file.txt\n", out);
  fputs("  table a.txt b.txt\n", out);
  fputs("  table -- -dashfile\n", out);
}