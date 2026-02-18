// builtin_fields.c - `fields` loadable builtin (placeholder for Week 1)

#include "diamondcore.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Bash loadable builtin headers (provided by bash source / headers)
#include "config.h"
#include "builtins.h"
#include "shell.h"

void dc_print_usage_fields(FILE *out);

static char *fields_doc[] = {
  "Select and emit fields (placeholder).",
  "Week 1: not implemented yet.",
  (char *)0,
};

__attribute__((visibility("default")))
int fields_builtin(WORD_LIST *list) {
  // Minimal option handling: only --help recognized
  if (list && list->word && list->word->word &&
      strcmp(list->word->word, "--help") == 0) {
    dc_print_usage_fields(stdout);
    return 0;
  }

  fprintf(stderr, "fields: not implemented\n");
  return 2;
}

__attribute__((visibility("default")))
struct builtin fields_struct = {
  .name = "fields",
  .function = fields_builtin,
  .flags = BUILTIN_ENABLED,
  .long_doc = fields_doc,
  .short_doc = (char *)"fields SPEC [--] [FILE...]",
  .handle = 0,
};
