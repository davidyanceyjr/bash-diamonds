// help_take.c - help printer for `take`

#include "diamondcore.h"

#include <stdio.h>

void dc_print_help_take(FILE *out) {
  if (!out) out = stdout;

  fputs("usage: take N [S] [--] [FILE...]\n", out);
  fputs("       take --help\n", out);
  fputs("\n", out);
  fputs("Emit a forward-only slice of input lines.\n", out);
  fputs("take N emits the first N lines; take N S skips S lines then emits N lines.\n", out);
  fputs("\n", out);

  fputs("Options:\n", out);
  fputs("  --help   print this help and exit 0\n", out);
  fputs("  --       end option parsing\n", out);
  fputs("\n", out);

  fputs("Arguments:\n", out);
  fputs("  N        number of lines to emit (uint64, base-10, 0 allowed)\n", out);
  fputs("  S        number of lines to skip (uint64, base-10, 0 allowed; default 0)\n", out);
  fputs("  FILE...  optional inputs\n", out);
  fputs("\n", out);

  fputs("Numeric rules:\n", out);
  fputs("  - digits only; no sign\n", out);
  fputs("  - no leading zeros unless the value is exactly '0'\n", out);
  fputs("  - overflow is an error\n", out);
  fputs("\n", out);

  dc_print_help_common_files(out);
  fputs("\n", out);

  fputs("Output:\n", out);
  fputs("  - Selected lines are written verbatim (including their newline if present).\n", out);
  fputs("  - No newline is synthesized.\n", out);
  fputs("  - Stops reading once N lines have been emitted.\n", out);
  fputs("\n", out);

  dc_print_help_common_exit_codes(out);
  fputs("\n", out);
  dc_print_help_common_sigpipe(out);
  fputs("\n", out);

  fputs("Examples:\n", out);
  fputs("  take 10\n", out);
  fputs("  cmd | take 5 1\n", out);
  fputs("  take 20 10 -- a.txt - b.txt\n", out);
  fputs("  take 0 file.txt\n", out);
}