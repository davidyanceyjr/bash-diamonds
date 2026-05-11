// help_match.c - help printer for `match`

#include "diamondcore.h"

#include <stdio.h>

void dc_print_help_match(FILE *out) {
  if (!out)
    out = stdout;

  fputs("usage: match PATTERN [--] [FILE...]\n", out);
  fputs("       match --help\n", out);
  fputs("\n", out);
  fputs("Filter input lines by a deterministic, constrained regex.\n", out);
  fputs("Matches are evaluated on the line content excluding a terminating "
        "'\\n'.\n",
        out);
  fputs("\n", out);

  fputs("Options:\n", out);
  fputs("  --help   print this help and exit 0\n", out);
  fputs("  --       end option parsing\n", out);
  fputs("\n", out);

  fputs("Arguments:\n", out);
  fputs("  PATTERN  constrained regex pattern\n", out);
  fputs("  FILE...  optional inputs\n", out);
  fputs("\n", out);

  fputs("Pattern features (subset):\n", out);
  fputs("  Literals, '.', character classes [...], grouping (...), alternation "
        "'|'.\n",
        out);
  fputs("  Quantifiers: '*', '+', '?'.\n", out);
  fputs("  Anchors: '^' at start, '$' at end (otherwise literal unless "
        "escaped).\n",
        out);
  fputs(
      "  Only specific escapes are accepted (e.g. \\., \\[, \\], \\^, \\$).\n",
      out);
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
  fputs("  - Pattern compile errors and execution limit exceeded return exit "
        "2.\n",
        out);
  fputs("\n", out);

  dc_print_help_common_exit_codes(out);
  fputs("\n", out);
  dc_print_help_common_sigpipe(out);
  fputs("\n", out);

  fputs("Examples:\n", out);
  fputs("  match 'foo' file.txt\n", out);
  fputs("  cmd | match '^ERROR:'\n", out);
  fputs("  match '[0-9]+' -- -dashfile\n", out);
}