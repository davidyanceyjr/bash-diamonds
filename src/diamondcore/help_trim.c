// help_trim.c - help printer for `trim`

#include "diamondcore.h"

#include <stdio.h>

void dc_print_help_trim(FILE *out) {
  if (!out) out = stdout;

  fputs("usage: trim [--] [FILE...]\n", out);
  fputs("       trim --help\n", out);
  fputs("\n", out);
  fputs("Remove leading and trailing ASCII whitespace from each input line.\n", out);
  fputs("Lines that become empty after trimming are not emitted.\n", out);
  fputs("\n", out);

  fputs("Options:\n", out);
  fputs("  --help   print this help and exit 0\n", out);
  fputs("  --       end option parsing\n", out);
  fputs("\n", out);

  fputs("Arguments:\n", out);
  fputs("  FILE...  optional inputs\n", out);
  fputs("\n", out);

  dc_print_help_common_files(out);
  fputs("\n", out);

  fputs("Trimming:\n", out);
  fputs("  - Trims ASCII: space, tab, CR, VT, FF.\n", out);
  fputs("  - Newline is structural and preserved only when a line is emitted.\n", out);
  fputs("\n", out);

  fputs("Output:\n", out);
  fputs("  - If trimmed content is non-empty: emits it; preserves trailing newline if present.\n", out);
  fputs("  - If trimmed content is empty: emits nothing for that line.\n", out);
  fputs("\n", out);

  dc_print_help_common_exit_codes(out);
  fputs("\n", out);
  dc_print_help_common_sigpipe(out);
  fputs("\n", out);

  fputs("Examples:\n", out);
  fputs("  printf '  a  \\n' | trim\n", out);
  fputs("  trim file.txt\n", out);
  fputs("  cmd | trim | match '^x'\n", out);
}