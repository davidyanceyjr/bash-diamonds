/*
 * shell.h (vendored minimal)
 *
 * This project builds Bash loadable builtins. Normally these types come from
 * the Bash source headers. Many environments (including CI containers) do not
 * ship them.
 *
 * The definitions below are the minimal subset required by our builtins and are
 * consistent with Bash 5.2's loadable builtin ABI.
 */

#ifndef BD_VENDORED_SHELL_H
#define BD_VENDORED_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct word_desc {
  char *word;
  int flags;
} WORD_DESC;

typedef struct word_list {
  struct word_list *next;
  WORD_DESC *word;
} WORD_LIST;

#ifdef __cplusplus
}
#endif

#endif /* BD_VENDORED_SHELL_H */