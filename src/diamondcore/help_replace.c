/* src/diamondcore/help_replace.c */
// help_replace.c - help printer for `replace`

#include "diamondcore.h"

#include <stdio.h>

void dc_print_help_replace(FILE *out) {
  if (!out) out = stdout;

  fputs("usage: replace [--literal] PATTERN REPLACEMENT [--] [FILE...]\n", out);
  fputs("       replace --help\n", out);
  fputs("\n", out);

  fputs("Perform per-line substitution and emit only modified lines.\n", out);
  fputs("\n", out);

  fputs("Description:\n", out);
  fputs("  replace processes input record-by-record (records are delimited by 0x0A when present).\n", out);
  fputs("  Matching and replacement operate on the record content excluding a terminating '\\n'.\n", out);
  fputs("  If a record changes (>=1 substitution), the modified record is written; otherwise it is skipped.\n", out);
  fputs("\n", out);

  fputs("Options:\n", out);
  fputs("  --literal   Treat PATTERN as a literal byte sequence (no regex metacharacters).\n", out);
  fputs("  --help      Show this help and exit.\n", out);
  fputs("  --          End option parsing.\n", out);
  fputs("\n", out);

  fputs("Exit codes:\n", out);
  fputs("  0  At least one modified record was written.\n", out);
  fputs("  1  No substitutions occurred (no output).\n", out);
  fputs("  2  Usage error or runtime error (including I/O and write failures).\n", out);
  fputs("\n", out);

  fputs("Streaming model:\n", out);
  fputs("  Only the current record is buffered; the entire input is never buffered.\n", out);
  fputs("\n", out);

  fputs("SIGPIPE:\n", out);
  fputs("  SIGPIPE is ignored internally. If writing to stdout fails, replace exits 2.\n", out);
}