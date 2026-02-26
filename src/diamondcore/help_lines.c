// help_lines.c - help printer for `lines`

#include "diamondcore.h"

#include <stdio.h>

void dc_print_help_lines(FILE *out) {
  if (!out) out = stdout;

  fputs("usage: lines SPEC [--] [FILE...]\n", out);
  fputs("       lines --help\n", out);
  fputs("\n", out);
  fputs("Select and emit specific 1-based input lines by numeric index or range.\n", out);
  fputs("\n", out);

  fputs("Options:\n", out);
  fputs("  --help   print this help and exit 0\n", out);
  fputs("  --       end option parsing\n", out);
  fputs("\n", out);

  fputs("Arguments:\n", out);
  fputs("  SPEC     line selection (shared range grammar)\n", out);
  fputs("  FILE...  optional inputs\n", out);
  fputs("\n", out);

  dc_print_help_common_range_spec(out, "SPEC", "Applies to the concatenation of all inputs.");
  fputs("\n", out);

  dc_print_help_common_files(out);
  fputs("\n", out);

  fputs("Output:\n", out);
  fputs("  - Matching lines are written verbatim (including their newline if present).\n", out);
  fputs("  - No newline is synthesized.\n", out);
  fputs("\n", out);

  dc_print_help_common_exit_codes(out);
  fputs("\n", out);
  dc_print_help_common_sigpipe(out);
  fputs("\n", out);

  fputs("Examples:\n", out);
  fputs("  lines 3\n", out);
  fputs("  lines 1,3 file.txt\n", out);
  fputs("  cmd | lines 2..\n", out);
  fputs("  lines '..2' -- -dashfile\n", out);
}