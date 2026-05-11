// builtin_filter.c - `filter` loadable builtin

#include "diamondcore.h"

#include <signal.h> // ANCHOR:SIGPIPE-INCLUDE
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "shell.h"

__attribute__((unused)) static const char *filter_shortdoc =
    "filter EXPR [--] [FILE...]";

static char *filter_doc[] = {
    "Select input lines whose fields satisfy a constrained boolean expression.",
    (char *)0,
};

/* =============================================================================
 * Errors / help
 * =============================================================================
 */

static int filter_usage_err(const char *msg) {
  if (msg && *msg)
    fprintf(stderr, "filter: %s\n", msg);
  else
    dc_print_usage_filter(stderr);
  return 2;
}

static int filter_io_err(const char *msg) {
  if (msg && *msg)
    fprintf(stderr, "filter: %s\n", msg);
  else
    fprintf(stderr, "filter: I/O error\n");
  return 2;
}

static int filter_help(void) {
  dc_print_help_filter(stdout);
  return 0;
}

/* =============================================================================
 * Expression engine (minimal v1 per docs/filter.md)
 * =============================================================================
 */

enum tok_type {
  TOK_EOF = 0,
  TOK_LPAREN,
  TOK_RPAREN,
  TOK_AND,
  TOK_OR,
  TOK_NOT,
  TOK_EQ,
  TOK_NE,
  TOK_LT,
  TOK_LE,
  TOK_GT,
  TOK_GE,
  TOK_FIELD,
  TOK_INT,
  TOK_STR,
};

typedef struct {
  enum tok_type t;
  const char *start;
  size_t len;
  uint64_t u64; /* for TOK_FIELD */
  int64_t i64;  /* for TOK_INT */
  char *owned;  /* for TOK_STR (unescaped), and TOK_INT (lexeme copy) */
  size_t owned_len;
} tok_t;

typedef struct {
  const char *s;
  size_t len;
  size_t i;
  size_t token_count;
  char err[256];
} lexer_t;

static bool lex_err(lexer_t *lx, const char *msg) {
  if (!lx)
    return false;
  snprintf(lx->err, sizeof(lx->err), "%s",
           msg ? msg : "expression parse error");
  return false;
}

static inline bool is_ws_ch(unsigned char c) {
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
          c == '\f');
}

static inline bool is_digit_ch(unsigned char c) {
  return (c >= '0' && c <= '9');
}

static bool parse_i64_bytes(const uint8_t *p, size_t n, int64_t *out) {
  if (!p || n == 0 || !out)
    return false;
  size_t i = 0;
  bool neg = false;
  if (p[i] == '-') {
    neg = true;
    i++;
    if (i >= n)
      return false;
  }
  uint64_t acc = 0;
  for (; i < n; i++) {
    if (!is_digit_ch((unsigned char)p[i]))
      return false;
    uint64_t d = (uint64_t)(p[i] - '0');
    if (acc > UINT64_MAX / 10ULL)
      return false;
    acc *= 10ULL;
    if (acc > UINT64_MAX - d)
      return false;
    acc += d;
  }

  if (neg) {
    /* Allow INT64_MIN exactly: magnitude 9223372036854775808 */
    if (acc > (uint64_t)INT64_MAX + 1ULL)
      return false;
    if (acc == (uint64_t)INT64_MAX + 1ULL) {
      *out = INT64_MIN;
      return true;
    }
    *out = -(int64_t)acc;
    return true;
  }

  if (acc > (uint64_t)INT64_MAX)
    return false;
  *out = (int64_t)acc;
  return true;
}

static bool parse_i64_cstr(const char *s, int64_t *out) {
  if (!s)
    return false;
  return parse_i64_bytes((const uint8_t *)s, strlen(s), out);
}

static int bytes_cmp(const uint8_t *a, size_t alen, const uint8_t *b,
                     size_t blen) {
  size_t n = (alen < blen) ? alen : blen;
  for (size_t i = 0; i < n; i++) {
    unsigned int av = (unsigned int)a[i];
    unsigned int bv = (unsigned int)b[i];
    if (av < bv)
      return -1;
    if (av > bv)
      return 1;
  }
  if (alen < blen)
    return -1;
  if (alen > blen)
    return 1;
  return 0;
}

static void tok_free(tok_t *t) {
  if (!t)
    return;
  free(t->owned);
  t->owned = NULL;
  t->owned_len = 0;
}

static bool lex_next(lexer_t *lx, tok_t *out) {
  if (!lx || !out)
    return false;
  memset(out, 0, sizeof(*out));
  out->t = TOK_EOF;

  /* limits */
  if (lx->token_count >= 2048)
    return lex_err(lx, "expression too complex");

  while (lx->i < lx->len && is_ws_ch((unsigned char)lx->s[lx->i]))
    lx->i++;
  if (lx->i >= lx->len) {
    out->t = TOK_EOF;
    lx->token_count++;
    return true;
  }

  const char *p = lx->s + lx->i;
  size_t rem = lx->len - lx->i;

  /* Two-char operators */
  if (rem >= 2) {
    if (p[0] == '&' && p[1] == '&') {
      out->t = TOK_AND;
      out->start = p;
      out->len = 2;
      lx->i += 2;
      lx->token_count++;
      return true;
    }
    if (p[0] == '|' && p[1] == '|') {
      out->t = TOK_OR;
      out->start = p;
      out->len = 2;
      lx->i += 2;
      lx->token_count++;
      return true;
    }
    if (p[0] == '=' && p[1] == '=') {
      out->t = TOK_EQ;
      out->start = p;
      out->len = 2;
      lx->i += 2;
      lx->token_count++;
      return true;
    }
    if (p[0] == '!' && p[1] == '=') {
      out->t = TOK_NE;
      out->start = p;
      out->len = 2;
      lx->i += 2;
      lx->token_count++;
      return true;
    }
    if (p[0] == '<' && p[1] == '=') {
      out->t = TOK_LE;
      out->start = p;
      out->len = 2;
      lx->i += 2;
      lx->token_count++;
      return true;
    }
    if (p[0] == '>' && p[1] == '=') {
      out->t = TOK_GE;
      out->start = p;
      out->len = 2;
      lx->i += 2;
      lx->token_count++;
      return true;
    }
  }

  /* One-char tokens */
  switch (p[0]) {
  case '(':
    out->t = TOK_LPAREN;
    out->start = p;
    out->len = 1;
    lx->i += 1;
    lx->token_count++;
    return true;
  case ')':
    out->t = TOK_RPAREN;
    out->start = p;
    out->len = 1;
    lx->i += 1;
    lx->token_count++;
    return true;
  case '!':
    out->t = TOK_NOT;
    out->start = p;
    out->len = 1;
    lx->i += 1;
    lx->token_count++;
    return true;
  case '<':
    out->t = TOK_LT;
    out->start = p;
    out->len = 1;
    lx->i += 1;
    lx->token_count++;
    return true;
  case '>':
    out->t = TOK_GT;
    out->start = p;
    out->len = 1;
    lx->i += 1;
    lx->token_count++;
    return true;
  default:
    break;
  }

  /* Field reference: $<digits> (must be >0) */
  if (p[0] == '$') {
    if (rem < 2 || !is_digit_ch((unsigned char)p[1]))
      return lex_err(lx, "invalid field reference");
    size_t j = 1;
    uint64_t v = 0;
    while (j < rem && is_digit_ch((unsigned char)p[j])) {
      uint64_t d = (uint64_t)(p[j] - '0');
      if (v > UINT64_MAX / 10ULL)
        return lex_err(lx, "field index overflow");
      v *= 10ULL;
      if (v > UINT64_MAX - d)
        return lex_err(lx, "field index overflow");
      v += d;
      j++;
    }
    if (v == 0)
      return lex_err(lx, "$0 is invalid");
    out->t = TOK_FIELD;
    out->start = p;
    out->len = j;
    out->u64 = v;
    lx->i += j;
    lx->token_count++;
    return true;
  }

  /* String literal "..." with only \\ and \" escapes */
  if (p[0] == '"') {
    size_t j = 1;
    char *buf = NULL;
    size_t cap = 0;
    size_t cnt = 0;
    for (;;) {
      if (lx->i + j >= lx->len) {
        free(buf);
        return lex_err(lx, "unterminated string literal");
      }
      unsigned char c = (unsigned char)p[j];
      if (c == '"') {
        j++;
        break;
      }
      if (c == '\\') {
        if (lx->i + j + 1 >= lx->len) {
          free(buf);
          return lex_err(lx, "unterminated string literal");
        }
        unsigned char n = (unsigned char)p[j + 1];
        if (n == '\\' || n == '"') {
          c = n;
          j += 2;
        } else {
          free(buf);
          return lex_err(lx, "invalid escape in string literal");
        }
      } else {
        j++;
      }

      if (cnt == cap) {
        size_t ncap = cap ? cap * 2 : 16;
        char *nb = (char *)realloc(buf, ncap);
        if (!nb) {
          free(buf);
          return lex_err(lx, "out of memory");
        }
        buf = nb;
        cap = ncap;
      }
      buf[cnt++] = (char)c;
    }
    out->t = TOK_STR;
    out->start = p;
    out->len = j;
    out->owned = buf;
    out->owned_len = cnt;
    lx->i += j;
    lx->token_count++;
    return true;
  }

  /* Integer literal: -?[0-9]+ */
  if (p[0] == '-' || is_digit_ch((unsigned char)p[0])) {
    size_t j = 0;
    if (p[0] == '-') {
      j++;
      if (j >= rem || !is_digit_ch((unsigned char)p[j]))
        return lex_err(lx, "invalid integer literal");
    }
    while (j < rem && is_digit_ch((unsigned char)p[j]))
      j++;
    char *lex = (char *)malloc(j + 1);
    if (!lex)
      return lex_err(lx, "out of memory");
    memcpy(lex, p, j);
    lex[j] = '\0';
    int64_t iv = 0;
    if (!parse_i64_cstr(lex, &iv)) {
      free(lex);
      return lex_err(lx, "integer literal out of range");
    }
    out->t = TOK_INT;
    out->start = p;
    out->len = j;
    out->i64 = iv;
    out->owned = lex;
    out->owned_len = j;
    lx->i += j;
    lx->token_count++;
    return true;
  }

  return lex_err(lx, "unexpected token");
}

enum opnd_type { OP_FIELD = 1, OP_INT = 2, OP_STR = 3 };
typedef struct {
  enum opnd_type t;
  uint64_t field_idx; /* 1-based */
  int64_t i64;
  char *s; /* owned for OP_INT (lexeme) and OP_STR (unescaped) */
  size_t slen;
} operand_t;

enum cmp_op {
  CMP_EQ,
  CMP_NE,
  CMP_LT,
  CMP_LE,
  CMP_GT,
  CMP_GE,
};

enum node_type { N_AND = 1, N_OR = 2, N_NOT = 3, N_CMP = 4 };

typedef struct node {
  enum node_type t;
  struct node *a;
  struct node *b;
  enum cmp_op cop;
  operand_t l;
  operand_t r;
} node_t;

typedef struct {
  lexer_t lx;
  tok_t cur;
  size_t node_count;
  char err[256];
} parser_t;

static void operand_free(operand_t *o) {
  if (!o)
    return;
  free(o->s);
  o->s = NULL;
  o->slen = 0;
}

static void node_free(node_t *n) {
  if (!n)
    return;
  if (n->t == N_AND || n->t == N_OR || n->t == N_NOT) {
    node_free(n->a);
    node_free(n->b);
  }
  if (n->t == N_CMP) {
    operand_free(&n->l);
    operand_free(&n->r);
  }
  free(n);
}

static bool parser_err(parser_t *ps, const char *msg) {
  if (!ps)
    return false;
  snprintf(ps->err, sizeof(ps->err), "%s",
           msg ? msg : "expression parse error");
  return false;
}

static bool ps_next(parser_t *ps) {
  if (!ps)
    return false;
  tok_free(&ps->cur);
  if (!lex_next(&ps->lx, &ps->cur)) {
    snprintf(ps->err, sizeof(ps->err), "%s",
             ps->lx.err[0] ? ps->lx.err : "expression parse error");
    return false;
  }
  return true;
}

static node_t *ps_new_node(parser_t *ps, enum node_type t) {
  if (!ps)
    return NULL;
  if (ps->node_count >= 2048) {
    parser_err(ps, "expression too complex");
    return NULL;
  }
  node_t *n = (node_t *)calloc(1, sizeof(*n));
  if (!n) {
    parser_err(ps, "out of memory");
    return NULL;
  }
  n->t = t;
  ps->node_count++;
  return n;
}

static bool ps_expect(parser_t *ps, enum tok_type t, const char *msg) {
  if (!ps)
    return false;
  if (ps->cur.t != t)
    return parser_err(ps, msg);
  return ps_next(ps);
}

static bool ps_parse_operand(parser_t *ps, operand_t *out) {
  if (!ps || !out)
    return false;
  memset(out, 0, sizeof(*out));

  if (ps->cur.t == TOK_FIELD) {
    out->t = OP_FIELD;
    out->field_idx = ps->cur.u64;
    return ps_next(ps);
  }
  if (ps->cur.t == TOK_INT) {
    out->t = OP_INT;
    out->i64 = ps->cur.i64;
    out->s = ps->cur.owned;
    out->slen = ps->cur.owned_len;
    ps->cur.owned = NULL;
    ps->cur.owned_len = 0;
    return ps_next(ps);
  }
  if (ps->cur.t == TOK_STR) {
    out->t = OP_STR;
    out->s = ps->cur.owned;
    out->slen = ps->cur.owned_len;
    ps->cur.owned = NULL;
    ps->cur.owned_len = 0;
    return ps_next(ps);
  }

  return parser_err(ps, "missing operand");
}

static bool tok_is_cmp(enum tok_type t) {
  return (t == TOK_EQ || t == TOK_NE || t == TOK_LT || t == TOK_LE ||
          t == TOK_GT || t == TOK_GE);
}

static enum cmp_op tok_to_cmp(enum tok_type t) {
  switch (t) {
  case TOK_EQ:
    return CMP_EQ;
  case TOK_NE:
    return CMP_NE;
  case TOK_LT:
    return CMP_LT;
  case TOK_LE:
    return CMP_LE;
  case TOK_GT:
    return CMP_GT;
  case TOK_GE:
    return CMP_GE;
  default:
    return CMP_EQ;
  }
}

static node_t *ps_parse_expr(parser_t *ps);

static node_t *ps_parse_primary(parser_t *ps) {
  if (!ps)
    return NULL;

  if (ps->cur.t == TOK_LPAREN) {
    if (!ps_next(ps))
      return NULL;
    node_t *inner = ps_parse_expr(ps);
    if (!inner)
      return NULL;
    if (!ps_expect(ps, TOK_RPAREN, "missing ')'")) {
      node_free(inner);
      return NULL;
    }
    return inner;
  }

  /* comparison */
  operand_t l;
  operand_t r;
  if (!ps_parse_operand(ps, &l))
    return NULL;
  if (!tok_is_cmp(ps->cur.t)) {
    operand_free(&l);
    parser_err(ps, "missing comparison operator");
    return NULL;
  }
  enum cmp_op cop = tok_to_cmp(ps->cur.t);
  if (!ps_next(ps)) {
    operand_free(&l);
    return NULL;
  }
  if (!ps_parse_operand(ps, &r)) {
    operand_free(&l);
    return NULL;
  }

  node_t *n = ps_new_node(ps, N_CMP);
  if (!n) {
    operand_free(&l);
    operand_free(&r);
    return NULL;
  }
  n->cop = cop;
  n->l = l;
  n->r = r;
  return n;
}

static node_t *ps_parse_unary(parser_t *ps) {
  if (!ps)
    return NULL;
  if (ps->cur.t == TOK_NOT) {
    if (!ps_next(ps))
      return NULL;
    node_t *inner = ps_parse_unary(ps);
    if (!inner)
      return NULL;
    node_t *n = ps_new_node(ps, N_NOT);
    if (!n) {
      node_free(inner);
      return NULL;
    }
    n->a = inner;
    return n;
  }
  return ps_parse_primary(ps);
}

static node_t *ps_parse_and(parser_t *ps) {
  if (!ps)
    return NULL;
  node_t *left = ps_parse_unary(ps);
  if (!left)
    return NULL;
  while (ps->cur.t == TOK_AND) {
    if (!ps_next(ps)) {
      node_free(left);
      return NULL;
    }
    node_t *right = ps_parse_unary(ps);
    if (!right) {
      node_free(left);
      return NULL;
    }
    node_t *n = ps_new_node(ps, N_AND);
    if (!n) {
      node_free(left);
      node_free(right);
      return NULL;
    }
    n->a = left;
    n->b = right;
    left = n;
  }
  return left;
}

static node_t *ps_parse_or(parser_t *ps) {
  if (!ps)
    return NULL;
  node_t *left = ps_parse_and(ps);
  if (!left)
    return NULL;
  while (ps->cur.t == TOK_OR) {
    if (!ps_next(ps)) {
      node_free(left);
      return NULL;
    }
    node_t *right = ps_parse_and(ps);
    if (!right) {
      node_free(left);
      return NULL;
    }
    node_t *n = ps_new_node(ps, N_OR);
    if (!n) {
      node_free(left);
      node_free(right);
      return NULL;
    }
    n->a = left;
    n->b = right;
    left = n;
  }
  return left;
}

static node_t *ps_parse_expr(parser_t *ps) { return ps_parse_or(ps); }

static node_t *filter_parse_expr(const char *expr, char err[256]) {
  if (err)
    err[0] = '\0';
  if (!expr || !*expr) {
    if (err)
      snprintf(err, 256, "empty expression");
    return NULL;
  }

  size_t elen = strlen(expr);
  if (elen > 4096) {
    if (err)
      snprintf(err, 256, "expression too long");
    return NULL;
  }

  parser_t ps;
  memset(&ps, 0, sizeof(ps));
  ps.lx.s = expr;
  ps.lx.len = elen;
  ps.lx.i = 0;
  ps.lx.token_count = 0;
  ps.lx.err[0] = '\0';
  ps.err[0] = '\0';

  /* prime */
  if (!ps_next(&ps)) {
    if (err)
      snprintf(err, 256, "%s", ps.err[0] ? ps.err : "expression parse error");
    tok_free(&ps.cur);
    return NULL;
  }

  node_t *root = ps_parse_expr(&ps);
  if (!root) {
    if (err)
      snprintf(err, 256, "%s", ps.err[0] ? ps.err : "expression parse error");
    tok_free(&ps.cur);
    return NULL;
  }

  if (ps.cur.t != TOK_EOF) {
    node_free(root);
    if (err)
      snprintf(err, 256, "unexpected token");
    tok_free(&ps.cur);
    return NULL;
  }

  tok_free(&ps.cur);
  return root;
}

typedef struct {
  const dc_field_view_t *fields;
  size_t field_count;
  uint64_t steps;
  uint64_t step_limit;
} eval_ctx_t;

static bool eval_operand_view(const operand_t *o, const uint8_t **p, size_t *n,
                              eval_ctx_t *ctx) {
  (void)ctx;
  if (!o || !p || !n)
    return false;
  *p = (const uint8_t *)"";
  *n = 0;
  switch (o->t) {
  case OP_FIELD: {
    uint64_t idx = o->field_idx;
    if (idx == 0) {
      *p = (const uint8_t *)"";
      *n = 0;
      return true;
    }
    size_t z = (size_t)(idx - 1);
    if (!ctx || z >= ctx->field_count) {
      *p = (const uint8_t *)"";
      *n = 0;
      return true;
    }
    *p = ctx->fields[z].ptr;
    *n = ctx->fields[z].len;
    return true;
  }
  case OP_INT:
    *p = (const uint8_t *)o->s;
    *n = o->slen;
    return true;
  case OP_STR:
    *p = (const uint8_t *)o->s;
    *n = o->slen;
    return true;
  default:
    return false;
  }
}

static bool eval_cmp(const node_t *n, eval_ctx_t *ctx, bool *exec_limit) {
  if (exec_limit)
    *exec_limit = false;
  if (!n || !ctx || !exec_limit)
    return false;

  const uint8_t *lp = NULL;
  size_t ln = 0;
  const uint8_t *rp = NULL;
  size_t rn = 0;
  if (!eval_operand_view(&n->l, &lp, &ln, ctx))
    return false;
  if (!eval_operand_view(&n->r, &rp, &rn, ctx))
    return false;

  int64_t li = 0;
  int64_t ri = 0;
  bool lnum = parse_i64_bytes(lp, ln, &li);
  bool rnum = parse_i64_bytes(rp, rn, &ri);

  if (lnum && rnum) {
    switch (n->cop) {
    case CMP_EQ:
      return li == ri;
    case CMP_NE:
      return li != ri;
    case CMP_LT:
      return li < ri;
    case CMP_LE:
      return li <= ri;
    case CMP_GT:
      return li > ri;
    case CMP_GE:
      return li >= ri;
    }
    return false;
  }

  int c = bytes_cmp(lp, ln, rp, rn);
  switch (n->cop) {
  case CMP_EQ:
    return c == 0;
  case CMP_NE:
    return c != 0;
  case CMP_LT:
    return c < 0;
  case CMP_LE:
    return c <= 0;
  case CMP_GT:
    return c > 0;
  case CMP_GE:
    return c >= 0;
  }
  return false;
}

static bool eval_bool(const node_t *n, eval_ctx_t *ctx, bool *exec_limit) {
  if (exec_limit)
    *exec_limit = false;
  if (!n || !ctx || !exec_limit)
    return false;

  ctx->steps++;
  if (ctx->steps > ctx->step_limit) {
    *exec_limit = true;
    return false;
  }

  switch (n->t) {
  case N_CMP:
    return eval_cmp(n, ctx, exec_limit);
  case N_NOT: {
    bool lim = false;
    bool v = eval_bool(n->a, ctx, &lim);
    if (lim) {
      *exec_limit = true;
      return false;
    }
    return !v;
  }
  case N_AND: {
    bool lim = false;
    bool lv = eval_bool(n->a, ctx, &lim);
    if (lim) {
      *exec_limit = true;
      return false;
    }
    if (!lv)
      return false;
    bool rv = eval_bool(n->b, ctx, &lim);
    if (lim) {
      *exec_limit = true;
      return false;
    }
    return rv;
  }
  case N_OR: {
    bool lim = false;
    bool lv = eval_bool(n->a, ctx, &lim);
    if (lim) {
      *exec_limit = true;
      return false;
    }
    if (lv)
      return true;
    bool rv = eval_bool(n->b, ctx, &lim);
    if (lim) {
      *exec_limit = true;
      return false;
    }
    return rv;
  }
  default:
    return false;
  }
}

/* =============================================================================
 * Main
 * =============================================================================
 */

static int filter_main(const char *expr, char *const *files,
                       size_t file_count) {
  char perr[256];
  node_t *ast = filter_parse_expr(expr, perr);
  if (!ast) {
    if (perr[0])
      fprintf(stderr, "filter: %s\n", perr);
    else
      fprintf(stderr, "filter: expression parse error\n");
    return 2;
  }

  dc_error_t err;
  dc_line_reader_t *lr = dc_lr_open(files, file_count, &err);
  if (!lr) {
    node_free(ast);
    return filter_io_err(err.msg[0] ? err.msg : "cannot open input");
  }

  bool emitted = false;
  const uint8_t delim = (uint8_t)'\t';

  for (;;) {
    dc_line_view_t v;
    bool ok = dc_lr_next(lr, &v, &err);
    if (!ok) {
      if (err.code != DC_ERR_NONE) {
        dc_lr_close(lr);
        node_free(ast);
        return filter_io_err(err.msg[0] ? err.msg : "read error");
      }
      break; /* EOF */
    }

    /* Exclude trailing '\n' from field content */
    size_t subj_len = v.len;
    if (v.ends_with_nl && subj_len > 0)
      subj_len--;

    dc_field_view_t *fields = NULL;
    size_t fcnt = dc_split_delim(v.ptr, subj_len, delim, &fields);
    if (fcnt == (size_t)-1) {
      dc_lr_close(lr);
      node_free(ast);
      return filter_io_err("out of memory");
    }

    eval_ctx_t ctx;
    ctx.fields = fields;
    ctx.field_count = fcnt;
    ctx.steps = 0;
    ctx.step_limit = 500000ULL;

    bool exec_limit = false;
    bool keep = eval_bool(ast, &ctx, &exec_limit);
    free(fields);

    if (exec_limit) {
      fprintf(stderr, "filter: expression evaluation limit exceeded\n");
      dc_lr_close(lr);
      node_free(ast);
      return 2;
    }

    if (keep) {
      if (v.len > 0) {
        size_t n = fwrite(v.ptr, 1, v.len, stdout);
        if (n != v.len || ferror(stdout)) {
          dc_lr_close(lr);
          node_free(ast);
          return filter_io_err("write error");
        }
        if (fflush(stdout) != 0 || ferror(stdout)) {
          dc_lr_close(lr);
          node_free(ast);
          return filter_io_err("write error");
        }
      }
      emitted = true;
    }
  }

  if (fflush(stdout) != 0 || ferror(stdout)) {
    dc_lr_close(lr);
    node_free(ast);
    return filter_io_err("write error");
  }

  dc_lr_close(lr);
  node_free(ast);
  return emitted ? 0 : 1;
}

/*
Parsing rules (Diamond style):
- Only --help is recognized.
- Any other -x token is an error unless after --, or token is exactly '-'.
- EXPR is required and is the first non-option token.
*/
__attribute__((visibility("default"))) int filter_builtin(WORD_LIST *list) {
  // === ANCHOR:SIGPIPE-BEGIN ===
  void (*old_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);
  // === ANCHOR:SIGPIPE-END ===

  bool end_opts = false;
  const char *expr = NULL;

  size_t fcap = 8;
  size_t fcnt = 0;
  char **files = (char **)calloc(fcap, sizeof(char *));
  if (!files) {
    signal(SIGPIPE, old_sigpipe);
    return filter_io_err("out of memory");
  }

  int rc = 2;
  for (WORD_LIST *w = list; w; w = w->next) {
    const char *tok = w->word->word;
    if (!tok)
      tok = "";

    if (!expr) {
      if (!end_opts && strcmp(tok, "--help") == 0) {
        rc = filter_help();
        goto out;
      }
      if (!end_opts && strcmp(tok, "--") == 0) {
        end_opts = true;
        continue;
      }
      if (!end_opts && tok[0] == '-' && tok[1] != '\0' &&
          strcmp(tok, "-") != 0) {
        rc = filter_usage_err("unknown option (use --help)");
        goto out;
      }
      expr = tok;
      continue;
    }

    if (!end_opts && strcmp(tok, "--") == 0) {
      end_opts = true;
      continue;
    }
    if (!end_opts && tok[0] == '-' && tok[1] != '\0' && strcmp(tok, "-") != 0) {
      rc = filter_usage_err("unknown option (use --help)");
      goto out;
    }

    if (fcnt == fcap) {
      size_t ncap = fcap * 2;
      char **nf = (char **)realloc(files, ncap * sizeof(char *));
      if (!nf) {
        rc = filter_io_err("out of memory");
        goto out;
      }
      files = nf;
      fcap = ncap;
    }
    files[fcnt++] = (char *)tok;
  }

  if (!expr) {
    rc = filter_usage_err("missing EXPR");
    goto out;
  }

  rc = filter_main(expr, files, fcnt);

out:
  free(files);
  signal(SIGPIPE, old_sigpipe);
  return rc;
}

__attribute__((visibility("default"))) struct builtin filter_struct = {
    .name = "filter",
    .function = filter_builtin,
    .flags = BUILTIN_ENABLED,
    .long_doc = filter_doc,
    .short_doc = (char *)"filter EXPR [--] [FILE...]",
    .handle = 0,
};