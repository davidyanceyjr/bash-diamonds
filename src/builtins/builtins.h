/*
 * builtins.h (vendored minimal)
 *
 * Minimal Bash loadable builtin interface.
 *
 * The runtime `enable -f` loader expects a global `struct builtin <name>_struct`
 * symbol with the layout below.
 */

#ifndef BD_VENDORED_BUILTINS_H
#define BD_VENDORED_BUILTINS_H

#include "shell.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int sh_builtin_func_t(WORD_LIST *);

/* Flags (subset). */
#define BUILTIN_ENABLED 0x01

struct builtin {
  char *name;
  sh_builtin_func_t *function;
  int flags;
  char **long_doc;
  char *short_doc;
  void *handle;
};

#ifdef __cplusplus
}
#endif

#endif /* BD_VENDORED_BUILTINS_H */