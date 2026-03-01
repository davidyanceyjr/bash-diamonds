// help_fields.c - help printer for `fields`

#include "diamondcore.h"

#include <stdio.h>

void dc_print_help_fields(FILE *out) {
  if (!out) out = stdout;

  fputs("usage: fields SPEC [FILE...]\n", out);
  fputs("       fields [--tsv] [-d DELIM] SPEC [--] [FILE...]\n", out);
  fputs("       fields --help\n", out);
  fputs("\n", out);
  fputs("Select and emit specific 1-based fields from each input line.\n", out);
  fputs("Default splitting is ASCII whitespace; -d enables single-byte delimiter splitting.\n", out);
  fputs("\n", out);

  fputs("Options:\n", out);
  fputs("  --help        print this help and exit 0\n", out);
  fputs("  --            end option parsing\n", out);
  fputs("  --tsv         join selected fields with a single TAB byte (0x09)\n", out);
  fputs("  -d DELIM      delimiter mode; DELIM must be exactly 1 byte\n", out);
  fputs("               accepted forms: -dX  or  -d X\n", out);
  fputs("\n", out);

  fputs("Arguments:\n", out);
  fputs("  SPEC          field selection (shared range grammar)\n", out);
  fputs("  FILE...       optional inputs\n", out);
  fputs("\n", out);

  dc_print_help_common_range_spec(out, "SPEC", "Applies independently to each input line.");
  fputs("\n", out);

  dc_print_help_common_files(out);
  fputs("\n", out);

  fputs("Splitting:\n", out);
  fputs("  Whitespace mode: ASCII whitespace delimits fields; runs collapse; no empty fields.\n", out);
  fputs("  -d mode: splits on DELIM; empty fields are preserved between consecutive delimiters.\n", out);
  fputs("\n", out);

  fputs("Output:\n", out);
  fputs("  - Selected fields are emitted in ascending order.\n", out);
  fputs("  - Join delimiter is a single space by default; with --tsv it is a single TAB byte.\n", out);
  fputs("  - If at least one field is selected for a line: emits that output line.\n", out);
  fputs("  - Output newline is preserved from the input line when an output line is emitted.\n", out);
  fputs("\n", out);

  dc_print_help_common_exit_codes(out);
  fputs("\n", out);
  dc_print_help_common_sigpipe(out);
  fputs("\n", out);

  fputs("Examples:\n", out);
  fputs("  fields 2\n", out);
  fputs("  fields 1,3 file.txt\n", out);
  fputs("  fields --tsv 2..3 file.txt\n", out);
  fputs("  fields -d: 1,7 /etc/passwd\n", out);
  fputs("  printf 'a::c\\n' | fields -d: 1..3\n", out);
}