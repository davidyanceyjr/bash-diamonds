// help_filter.c - help printer for `filter`

#include "diamondcore.h"

#include <stdio.h>

void dc_print_help_filter(FILE *out) {
  if (!out)
    out = stdout;

  fputs("usage: filter EXPR [--] [FILE...]\n", out);
  fputs("       filter --help\n", out);
  fputs("\n", out);
  fputs("Select input lines whose fields satisfy a constrained boolean "
        "expression.\n",
        out);
  fputs("Fields are delimited by a single TAB byte (0x09); no quoting, no "
        "trimming.\n",
        out);
  fputs("\n", out);

  fputs("Options:\n", out);
  fputs("  --help   print this help and exit 0\n", out);
  fputs("  --       end option parsing\n", out);
  fputs("\n", out);

  fputs("Arguments:\n", out);
  fputs("  EXPR     constrained boolean expression (see docs/filter.md)\n",
        out);
  fputs("  FILE...  optional inputs\n", out);
  fputs("\n", out);

  dc_print_help_common_files(out);
  fputs("\n", out);

  fputs("Output:\n", out);
  fputs("  - Matching lines are written verbatim (including their newline if "
        "present).\n",
        out);
  fputs("  - No newline is synthesized.\n", out);
  fputs("\n", out);

  fputs("Errors:\n", out);
  fputs("  - Expression parse errors, evaluation limit exceeded, I/O errors => "
        "exit 2.\n",
        out);
  fputs("\n", out);

  dc_print_help_common_exit_codes(out);
  fputs("\n", out);
  dc_print_help_common_sigpipe(out);
  fputs("\n", out);

  fputs("Examples:\n", out);
  fputs("  filter '$2 > 25' users.tsv\n", out);
  fputs("  match 'smith' users.tsv | filter '$2 > 25 && $3 > 83'\n", out);
}