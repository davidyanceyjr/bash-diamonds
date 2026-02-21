#include "dc_regex.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef enum {
  I_MATCH = 0,
  I_CHAR,
  I_ANY,
  I_CLASS,
  I_JMP,
  I_SPLIT,
  I_EOL
} op_t;

typedef struct {
  op_t op;
  int x;
  int y;
  uint16_t cls;
  uint8_t c;
} inst_t;

typedef struct {
  uint8_t bits[32]; /* 256-bit */
} cls_t;

struct dc_regex {
  inst_t *prog;
  int prog_len;

  cls_t *classes;
  int class_len;

  int start_pc;

  bool anchor_start;
  bool anchor_end;

  /* program allocated size fixed at max */
};

static void bitset_set(uint8_t bits[32], uint8_t b) { bits[b >> 3] |= (uint8_t)(1u << (b & 7)); }
static bool bitset_test(const uint8_t bits[32], uint8_t b) { return (bits[b >> 3] & (uint8_t)(1u << (b & 7))) != 0; }
static void bitset_invert(uint8_t bits[32]) { for (int i = 0; i < 32; i++) bits[i] = (uint8_t)~bits[i]; }

typedef struct {
  int *p;
  int n;
  int cap;
} plist_t;

static void plist_init(plist_t *pl) { pl->p = NULL; pl->n = 0; pl->cap = 0; }
static void plist_free(plist_t *pl) { free(pl->p); pl->p = NULL; pl->n = pl->cap = 0; }

static bool plist_push(plist_t *pl, int v) {
  if (pl->n == pl->cap) {
    int nc = (pl->cap == 0) ? 16 : pl->cap * 2;
    int *np = (int *)realloc(pl->p, (size_t)nc * sizeof(int));
    if (!np) return false;
    pl->p = np;
    pl->cap = nc;
  }
  pl->p[pl->n++] = v;
  return true;
}

static bool plist_append(plist_t *a, const plist_t *b) {
  for (int i = 0; i < b->n; i++) if (!plist_push(a, b->p[i])) return false;
  return true;
}

/* Patch encoding: (inst_index<<1) | field(0=x,1=y) */
static int patch_field(int inst_index, int field) { return (inst_index << 1) | (field & 1); }

typedef struct {
  int start;
  plist_t out;
  bool can_be_empty;
  bool valid;
} frag_t;

static frag_t frag_invalid(void) {
  frag_t f;
  f.start = -1;
  plist_init(&f.out);
  f.can_be_empty = false;
  f.valid = false;
  return f;
}
static frag_t frag_make(int start, plist_t out, bool can_be_empty) {
  frag_t f;
  f.start = start;
  f.out = out;
  f.can_be_empty = can_be_empty;
  f.valid = true;
  return f;
}

typedef struct {
  const char *pat;
  size_t len;
  size_t i;

  dc_regex_t *re;

  char *err; /* points to errbuf[256] */
  bool ok;

  int cls_cap;
  bool allow_empty;
} parser_t;

static void perr(parser_t *ps, const char *msg) {
  if (!ps->ok) return;
  ps->ok = false;
  if (ps->err) snprintf(ps->err, 256, "%s", msg);
}

static bool at_end(parser_t *ps) { return ps->i >= ps->len; }
static char peek(parser_t *ps) { return at_end(ps) ? '\0' : ps->pat[ps->i]; }
static char getc_ps(parser_t *ps) { return at_end(ps) ? '\0' : ps->pat[ps->i++]; }

static bool is_quant(char c) { return c == '*' || c == '+' || c == '?'; }

/* Treat unsupported constructs as compile error by making them meta here */
static bool is_meta(char c) {
  return c == '.' || c == '*' || c == '+' || c == '?' || c == '|' ||
         c == '(' || c == ')' || c == '[' || c == ']' || c == '^' ||
         c == '$' || c == '\\' || c == '{' || c == '}';
}

static int emit_inst(parser_t *ps, inst_t ins) {
  if (!ps->ok) return -1;
  if (ps->re->prog_len >= DC_REGEX_MAX_PROG_INSN) {
    perr(ps, "match: pattern compile error");
    return -1;
  }
  ps->re->prog[ps->re->prog_len] = ins;
  return ps->re->prog_len++;
}

static int add_class(parser_t *ps, const cls_t *c) {
  if (!ps->ok) return -1;
  if (ps->re->class_len == ps->cls_cap) {
    int nc = (ps->cls_cap == 0) ? 16 : ps->cls_cap * 2;
    cls_t *np = (cls_t *)realloc(ps->re->classes, (size_t)nc * sizeof(cls_t));
    if (!np) { perr(ps, "match: out of memory"); return -1; }
    ps->re->classes = np;
    ps->cls_cap = nc;
  }
  ps->re->classes[ps->re->class_len] = *c;
  return ps->re->class_len++;
}

static void patch(parser_t *ps, const plist_t *pl, int target) {
  for (int i = 0; i < pl->n; i++) {
    int enc = pl->p[i];
    int idx = enc >> 1;
    int fld = enc & 1;
    if (fld == 0) ps->re->prog[idx].x = target;
    else ps->re->prog[idx].y = target;
  }
}

/* only valid escapes: \. \* \+ \? \| \( \) \[ \] \^ \$ \\ */
static bool parse_escape_outside(parser_t *ps, uint8_t *out) {
  if (at_end(ps)) { perr(ps, "match: pattern compile error"); return false; }
  char c = getc_ps(ps);
  switch (c) {
    case '.': case '*': case '+': case '?': case '|':
    case '(': case ')': case '[': case ']': case '^':
    case '$': case '\\':
      *out = (uint8_t)c;
      return true;
    default:
      perr(ps, "match: pattern compile error");
      return false;
  }
}

static frag_t parse_alt(parser_t *ps);
static frag_t parse_concat(parser_t *ps);
static frag_t parse_repeat(parser_t *ps);
static frag_t parse_atom(parser_t *ps);

static frag_t parse_class(parser_t *ps) {
  cls_t cls;
  memset(&cls, 0, sizeof(cls));

  bool neg = false;
  bool have_any = false;

  if (at_end(ps)) { perr(ps, "match: pattern compile error"); return frag_invalid(); }
  if (peek(ps) == '^') { getc_ps(ps); neg = true; }

  if (at_end(ps) || peek(ps) == ']') { perr(ps, "match: pattern compile error"); return frag_invalid(); }

  int prev_raw = -1;
  int prev_atom = -1;
  bool prev_atom_valid_for_range = false;

  while (!at_end(ps)) {
    char c = getc_ps(ps);

    if (prev_raw == '[' && c == ':') { perr(ps, "match: pattern compile error"); return frag_invalid(); }
    prev_raw = (unsigned char)c;

    if (c == ']') {
      if (!have_any) { perr(ps, "match: pattern compile error"); return frag_invalid(); }
      break;
    }

    int atom = -1;
    if (c == '\\') {
      if (at_end(ps)) { perr(ps, "match: pattern compile error"); return frag_invalid(); }
      char e = getc_ps(ps);
      if (e == '\\' || e == ']' || e == '-' || e == '^') {
        atom = (unsigned char)e;
        prev_raw = (unsigned char)e;
      } else {
        perr(ps, "match: pattern compile error");
        return frag_invalid();
      }
    } else {
      atom = (unsigned char)c;
    }

    if (atom == '-' && prev_atom_valid_for_range) {
      if (at_end(ps)) { perr(ps, "match: pattern compile error"); return frag_invalid(); }
      if (peek(ps) == ']') {
        bitset_set(cls.bits, (uint8_t)'-');
        have_any = true;
        prev_atom = (unsigned char)'-';
        prev_atom_valid_for_range = true;
        continue;
      }

      int next_atom = -1;
      char nc = getc_ps(ps);

      if (prev_raw == '[' && nc == ':') { perr(ps, "match: pattern compile error"); return frag_invalid(); }
      prev_raw = (unsigned char)nc;

      if (nc == '\\') {
        if (at_end(ps)) { perr(ps, "match: pattern compile error"); return frag_invalid(); }
        char ne = getc_ps(ps);
        if (ne == '\\' || ne == ']' || ne == '-' || ne == '^') {
          next_atom = (unsigned char)ne;
          prev_raw = (unsigned char)ne;
        } else {
          perr(ps, "match: pattern compile error");
          return frag_invalid();
        }
      } else if (nc == ']') {
        bitset_set(cls.bits, (uint8_t)'-');
        have_any = true;
        break;
      } else {
        next_atom = (unsigned char)nc;
      }

      if (prev_atom > next_atom) { perr(ps, "match: pattern compile error"); return frag_invalid(); }
      for (int b = prev_atom; b <= next_atom; b++) bitset_set(cls.bits, (uint8_t)b);
      have_any = true;

      prev_atom = next_atom;
      prev_atom_valid_for_range = true;
      continue;
    }

    bitset_set(cls.bits, (uint8_t)atom);
    have_any = true;
    prev_atom = atom;
    prev_atom_valid_for_range = true;
  }

  if (neg) bitset_invert(cls.bits);

  int cls_id = add_class(ps, &cls);
  if (cls_id < 0) return frag_invalid();

  inst_t ins; memset(&ins, 0, sizeof(ins));
  ins.op = I_CLASS;
  ins.cls = (uint16_t)cls_id;
  ins.x = -1;
  int pc = emit_inst(ps, ins);
  if (pc < 0) return frag_invalid();

  plist_t out; plist_init(&out);
  if (!plist_push(&out, patch_field(pc, 0))) { plist_free(&out); perr(ps, "match: out of memory"); return frag_invalid(); }
  return frag_make(pc, out, false);
}

static frag_t parse_atom(parser_t *ps) {
  if (at_end(ps)) return frag_invalid();

  char c = peek(ps);
  if (c == ')' || c == '|') return frag_invalid();

  if (c == '(') {
    getc_ps(ps);
    if (at_end(ps) || peek(ps) == ')') { perr(ps, "match: pattern compile error"); return frag_invalid(); }
    frag_t inner = parse_alt(ps);
    if (!inner.valid || !ps->ok) return frag_invalid();
    if (at_end(ps) || peek(ps) != ')') { perr(ps, "match: pattern compile error"); plist_free(&inner.out); return frag_invalid(); }
    getc_ps(ps);
    return inner;
  }

  if (c == '[') { getc_ps(ps); return parse_class(ps); }

  if (c == '.') {
    getc_ps(ps);
    inst_t ins; memset(&ins, 0, sizeof(ins));
    ins.op = I_ANY; ins.x = -1;
    int pc = emit_inst(ps, ins);
    if (pc < 0) return frag_invalid();
    plist_t out; plist_init(&out);
    if (!plist_push(&out, patch_field(pc, 0))) { plist_free(&out); perr(ps, "match: out of memory"); return frag_invalid(); }
    return frag_make(pc, out, false);
  }

  if (c == '\\') {
    getc_ps(ps);
    uint8_t lit = 0;
    if (!parse_escape_outside(ps, &lit)) return frag_invalid();
    inst_t ins; memset(&ins, 0, sizeof(ins));
    ins.op = I_CHAR; ins.c = lit; ins.x = -1;
    int pc = emit_inst(ps, ins);
    if (pc < 0) return frag_invalid();
    plist_t out; plist_init(&out);
    if (!plist_push(&out, patch_field(pc, 0))) { plist_free(&out); perr(ps, "match: out of memory"); return frag_invalid(); }
    return frag_make(pc, out, false);
  }

  if (is_quant(c)) { perr(ps, "match: pattern compile error"); return frag_invalid(); }

  if (is_meta(c)) { perr(ps, "match: pattern compile error"); return frag_invalid(); }

  /* literal */
  getc_ps(ps);
  inst_t ins; memset(&ins, 0, sizeof(ins));
  ins.op = I_CHAR; ins.c = (uint8_t)c; ins.x = -1;
  int pc = emit_inst(ps, ins);
  if (pc < 0) return frag_invalid();
  plist_t out; plist_init(&out);
  if (!plist_push(&out, patch_field(pc, 0))) { plist_free(&out); perr(ps, "match: out of memory"); return frag_invalid(); }
  return frag_make(pc, out, false);
}

static frag_t parse_repeat(parser_t *ps) {
  size_t save = ps->i;
  frag_t a = parse_atom(ps);
  if (!a.valid) { ps->i = save; return frag_invalid(); }

  if (at_end(ps)) return a;
  char q = peek(ps);
  if (!is_quant(q)) return a;

  getc_ps(ps);

  if (!at_end(ps) && is_quant(peek(ps))) { perr(ps, "match: pattern compile error"); plist_free(&a.out); return frag_invalid(); }

  if (q == '?') {
    inst_t s; memset(&s, 0, sizeof(s));
    s.op = I_SPLIT; s.x = a.start; s.y = -1;
    int pc = emit_inst(ps, s);
    if (pc < 0) { plist_free(&a.out); return frag_invalid(); }

    plist_t out; plist_init(&out);
    if (!plist_push(&out, patch_field(pc, 1)) || !plist_append(&out, &a.out)) {
      plist_free(&out); plist_free(&a.out); perr(ps, "match: out of memory"); return frag_invalid();
    }
    plist_free(&a.out);
    return frag_make(pc, out, true);
  }

  if (q == '*') {
    inst_t s; memset(&s, 0, sizeof(s));
    s.op = I_SPLIT; s.x = a.start; s.y = -1;
    int pc = emit_inst(ps, s);
    if (pc < 0) { plist_free(&a.out); return frag_invalid(); }
    patch(ps, &a.out, pc);

    plist_t out; plist_init(&out);
    if (!plist_push(&out, patch_field(pc, 1))) { plist_free(&out); plist_free(&a.out); perr(ps, "match: out of memory"); return frag_invalid(); }
    plist_free(&a.out);
    return frag_make(pc, out, true);
  }

  /* '+' */
  inst_t s; memset(&s, 0, sizeof(s));
  s.op = I_SPLIT; s.x = a.start; s.y = -1;
  int pc = emit_inst(ps, s);
  if (pc < 0) { plist_free(&a.out); return frag_invalid(); }
  patch(ps, &a.out, pc);

  plist_t out; plist_init(&out);
  if (!plist_push(&out, patch_field(pc, 1))) { plist_free(&out); plist_free(&a.out); perr(ps, "match: out of memory"); return frag_invalid(); }
  plist_free(&a.out);
  return frag_make(a.start, out, false);
}

static frag_t parse_concat(parser_t *ps) {
  frag_t first = frag_invalid();
  bool have = false;

  while (!at_end(ps)) {
    char c = peek(ps);
    if (c == '|' || c == ')') break;

    size_t before = ps->i;
    frag_t r = parse_repeat(ps);
    if (!r.valid) {
      if (ps->i == before) perr(ps, "match: pattern compile error");
      break;
    }

    if (!have) {
      first = r;
      have = true;
    } else {
      patch(ps, &first.out, r.start);
      plist_free(&first.out);
      first.out = r.out;
      first.can_be_empty = first.can_be_empty && r.can_be_empty;
    }
  }

  if (!have) return frag_invalid();
  return first;
}

static frag_t parse_alt(parser_t *ps) {
  size_t start_i = ps->i;
  frag_t left = parse_concat(ps);

  bool saw_bar = false;
  while (!at_end(ps) && peek(ps) == '|') {
    saw_bar = true;
    getc_ps(ps);
    frag_t right = parse_concat(ps);
    if (!left.valid || !right.valid) {
      perr(ps, "match: pattern compile error");
      if (left.valid) plist_free(&left.out);
      if (right.valid) plist_free(&right.out);
      return frag_invalid();
    }

    inst_t s; memset(&s, 0, sizeof(s));
    s.op = I_SPLIT; s.x = left.start; s.y = right.start;
    int pc = emit_inst(ps, s);
    if (pc < 0) { plist_free(&left.out); plist_free(&right.out); return frag_invalid(); }

    plist_t out; plist_init(&out);
    if (!plist_append(&out, &left.out) || !plist_append(&out, &right.out)) {
      plist_free(&out); plist_free(&left.out); plist_free(&right.out); perr(ps, "match: out of memory"); return frag_invalid();
    }

    plist_free(&left.out);
    plist_free(&right.out);
    left = frag_make(pc, out, left.can_be_empty || right.can_be_empty);
  }

  if (!saw_bar && !left.valid) {
    if (ps->allow_empty && ps->i == start_i) {
      inst_t j; memset(&j, 0, sizeof(j));
      j.op = I_JMP; j.x = -1;
      int pc = emit_inst(ps, j);
      if (pc < 0) return frag_invalid();
      plist_t out; plist_init(&out);
      if (!plist_push(&out, patch_field(pc, 0))) { plist_free(&out); perr(ps, "match: out of memory"); return frag_invalid(); }
      return frag_make(pc, out, true);
    }
    perr(ps, "match: pattern compile error");
    return frag_invalid();
  }

  return left;
}

/* global anchor detection */
static bool is_escaped_at(const char *p, size_t idx) {
  size_t n = 0;
  while (idx > 0 && p[idx - 1] == '\\') { n++; idx--; }
  return (n & 1) == 1;
}

static void detect_global_anchors(const char *pat, size_t len,
                                  bool *out_bol, bool *out_eol,
                                  size_t *out_start, size_t *out_end) {
  *out_bol = false; *out_eol = false;
  *out_start = 0; *out_end = len;

  if (len == 0) return;

  if (pat[0] == '^' && !is_escaped_at(pat, 0)) {
    *out_bol = true;
    *out_start = 1;
  }

  bool in_class = false;
  size_t last = (size_t)-1;
  for (size_t i = 0; i < len; i++) {
    if (!is_escaped_at(pat, i)) {
      if (pat[i] == '[') in_class = true;
      else if (pat[i] == ']' && in_class) in_class = false;
      if (!in_class) last = i;
    }
  }

  if (last != (size_t)-1 && pat[last] == '$' && !is_escaped_at(pat, last)) {
    *out_eol = true;
    *out_end = last;
  }
}

/* Public API */

bool dc_regex_compile(dc_regex_t **out_re, const char *pattern, char errbuf[256]) {
  if (errbuf) errbuf[0] = '\0';
  if (!out_re || !pattern) return false;

  size_t plen = strlen(pattern);
  if (plen > DC_REGEX_MAX_PATTERN_LEN) {
    if (errbuf) snprintf(errbuf, 256, "match: pattern compile error");
    return false;
  }

  dc_regex_t *re = (dc_regex_t *)calloc(1, sizeof(dc_regex_t));
  if (!re) { if (errbuf) snprintf(errbuf, 256, "match: out of memory"); return false; }

  re->prog = (inst_t *)calloc(DC_REGEX_MAX_PROG_INSN, sizeof(inst_t));
  if (!re->prog) { free(re); if (errbuf) snprintf(errbuf, 256, "match: out of memory"); return false; }

  re->prog_len = 0;
  re->classes = NULL;
  re->class_len = 0;

  bool bol = false, eol = false;
  size_t start = 0, end = plen;
  detect_global_anchors(pattern, plen, &bol, &eol, &start, &end);
  re->anchor_start = bol;
  re->anchor_end = eol;

  const char *sub = pattern + start;
  size_t sublen = (end >= start) ? (end - start) : 0;

  parser_t ps;
  memset(&ps, 0, sizeof(ps));
  ps.pat = sub;
  ps.len = sublen;
  ps.i = 0;
  ps.re = re;
  ps.err = errbuf;
  ps.ok = true;
  ps.cls_cap = 0;
  ps.allow_empty = true;

  frag_t f;

  if (sublen == 0) {
    inst_t j; memset(&j, 0, sizeof(j));
    j.op = I_JMP; j.x = -1;
    int pc = emit_inst(&ps, j);
    if (pc < 0) { dc_regex_free(re); return false; }
    plist_t out; plist_init(&out);
    if (!plist_push(&out, patch_field(pc, 0))) { plist_free(&out); dc_regex_free(re); if (errbuf) snprintf(errbuf, 256, "match: out of memory"); return false; }
    f = frag_make(pc, out, true);
  } else {
    f = parse_alt(&ps);
    if (!ps.ok || !f.valid || ps.i != ps.len) {
      if (f.valid) plist_free(&f.out);
      dc_regex_free(re);
      if (errbuf && errbuf[0] == '\0') snprintf(errbuf, 256, "match: pattern compile error");
      return false;
    }
  }

  if (re->anchor_end) {
    inst_t e; memset(&e, 0, sizeof(e));
    e.op = I_EOL; e.x = -1;
    int epc = emit_inst(&ps, e);
    if (epc < 0) { plist_free(&f.out); dc_regex_free(re); return false; }
    patch(&ps, &f.out, epc);
    plist_free(&f.out);
    plist_init(&f.out);
    if (!plist_push(&f.out, patch_field(epc, 0))) { plist_free(&f.out); dc_regex_free(re); if (errbuf) snprintf(errbuf, 256, "match: out of memory"); return false; }
  }

  inst_t m; memset(&m, 0, sizeof(m));
  m.op = I_MATCH;
  int mpc = emit_inst(&ps, m);
  if (mpc < 0) { plist_free(&f.out); dc_regex_free(re); return false; }

  patch(&ps, &f.out, mpc);
  plist_free(&f.out);

  re->start_pc = f.start;
  *out_re = re;
  return true;
}

void dc_regex_free(dc_regex_t *re) {
  if (!re) return;
  free(re->prog);
  free(re->classes);
  free(re);
}

/* VM */

typedef struct { int pc; size_t pos; } work_t;

typedef struct {
  int *pcs;
  int n;
  int cap;
} slist_t;

static bool slist_init(slist_t *sl, int cap) {
  sl->pcs = (int *)malloc((size_t)cap * sizeof(int));
  if (!sl->pcs) return false;
  sl->n = 0; sl->cap = cap;
  return true;
}
static void slist_reset(slist_t *sl) { sl->n = 0; }
static void slist_free(slist_t *sl) { free(sl->pcs); sl->pcs = NULL; sl->n = sl->cap = 0; }
static bool slist_push(slist_t *sl, int pc) { if (sl->n >= sl->cap) return false; sl->pcs[sl->n++] = pc; return true; }

static bool list_has_match(const dc_regex_t *re, const slist_t *sl) {
  for (int i = 0; i < sl->n; i++) if (re->prog[sl->pcs[i]].op == I_MATCH) return true;
  return false;
}

static bool addstate(const dc_regex_t *re,
                     slist_t *dst,
                     uint32_t *mark,
                     uint32_t gen,
                     int pc,
                     size_t pos,
                     size_t subj_len,
                     uint64_t *steps,
                     bool *limit) {
  work_t stack[DC_REGEX_MAX_ACTIVE_STATES];
  int sp = 0;

  stack[sp++] = (work_t){ .pc = pc, .pos = pos };

  while (sp > 0) {
    work_t w = stack[--sp];

    (*steps)++;
    if (*steps > DC_REGEX_MAX_STEPS) { *limit = true; return false; }

    int cpc = w.pc;
    if (cpc < 0 || cpc >= re->prog_len) continue;
    if (mark[cpc] == gen) continue;
    mark[cpc] = gen;

    inst_t ins = re->prog[cpc];
    switch (ins.op) {
      case I_JMP:
        if (sp >= DC_REGEX_MAX_ACTIVE_STATES) { *limit = true; return false; }
        stack[sp++] = (work_t){ .pc = ins.x, .pos = w.pos };
        break;
      case I_SPLIT:
        if (sp + 2 >= DC_REGEX_MAX_ACTIVE_STATES) { *limit = true; return false; }
        stack[sp++] = (work_t){ .pc = ins.x, .pos = w.pos };
        stack[sp++] = (work_t){ .pc = ins.y, .pos = w.pos };
        break;
      case I_EOL:
        if (w.pos == subj_len) {
          if (sp >= DC_REGEX_MAX_ACTIVE_STATES) { *limit = true; return false; }
          stack[sp++] = (work_t){ .pc = ins.x, .pos = w.pos };
        }
        break;
      default:
        if (dst->n >= DC_REGEX_MAX_ACTIVE_STATES) { *limit = true; return false; }
        if (!slist_push(dst, cpc)) { *limit = true; return false; }
        break;
    }
  }

  return true;
}

bool dc_regex_match_line(const dc_regex_t *re,
                         const uint8_t *subject,
                         size_t subject_len,
                         bool *exec_limit_exceeded) {
  if (exec_limit_exceeded) *exec_limit_exceeded = false;
  if (!re) return false;

  uint32_t *mark = (uint32_t *)calloc((size_t)re->prog_len, sizeof(uint32_t));
  if (!mark) return false;

  slist_t clist, nlist;
  if (!slist_init(&clist, DC_REGEX_MAX_ACTIVE_STATES) ||
      !slist_init(&nlist, DC_REGEX_MAX_ACTIVE_STATES)) {
    free(mark);
    if (clist.pcs) slist_free(&clist);
    return false;
  }

  uint32_t gen = 1;
  uint64_t steps = 0;
  bool limit = false;

  slist_reset(&clist);
  slist_reset(&nlist);

  if (!addstate(re, &clist, mark, gen++, re->start_pc, 0, subject_len, &steps, &limit)) goto out;

  if (list_has_match(re, &clist)) { slist_free(&clist); slist_free(&nlist); free(mark); return true; }

  if (re->anchor_start) {
    for (size_t i = 0; i < subject_len; i++) {
      slist_reset(&nlist);

      for (int si = 0; si < clist.n; si++) {
        int pc = clist.pcs[si];

        steps++;
        if (steps > DC_REGEX_MAX_STEPS) { limit = true; break; }

        inst_t ins = re->prog[pc];
        uint8_t b = subject[i];

        if (ins.op == I_CHAR) {
          if (ins.c == b) if (!addstate(re, &nlist, mark, gen, ins.x, i + 1, subject_len, &steps, &limit)) break;
        } else if (ins.op == I_ANY) {
          if (!addstate(re, &nlist, mark, gen, ins.x, i + 1, subject_len, &steps, &limit)) break;
        } else if (ins.op == I_CLASS) {
          if (ins.cls < (uint16_t)re->class_len && bitset_test(re->classes[ins.cls].bits, b))
            if (!addstate(re, &nlist, mark, gen, ins.x, i + 1, subject_len, &steps, &limit)) break;
        }
      }

      if (limit) break;
      gen++;
      slist_t tmp = clist; clist = nlist; nlist = tmp;
      if (list_has_match(re, &clist)) { slist_free(&clist); slist_free(&nlist); free(mark); return true; }
      if (clist.n == 0) break;
    }
  } else {
    for (size_t i = 0; i < subject_len; i++) {
      slist_reset(&nlist);

      for (int si = 0; si < clist.n; si++) {
        int pc = clist.pcs[si];

        steps++;
        if (steps > DC_REGEX_MAX_STEPS) { limit = true; break; }

        inst_t ins = re->prog[pc];
        uint8_t b = subject[i];

        if (ins.op == I_CHAR) {
          if (ins.c == b) if (!addstate(re, &nlist, mark, gen, ins.x, i + 1, subject_len, &steps, &limit)) break;
        } else if (ins.op == I_ANY) {
          if (!addstate(re, &nlist, mark, gen, ins.x, i + 1, subject_len, &steps, &limit)) break;
        } else if (ins.op == I_CLASS) {
          if (ins.cls < (uint16_t)re->class_len && bitset_test(re->classes[ins.cls].bits, b))
            if (!addstate(re, &nlist, mark, gen, ins.x, i + 1, subject_len, &steps, &limit)) break;
        }
      }

      if (limit) break;

      /* restart NFA at next position */
      if (!addstate(re, &nlist, mark, gen, re->start_pc, i + 1, subject_len, &steps, &limit)) { /* may set limit */ }
      if (limit) break;

      gen++;
      slist_t tmp = clist; clist = nlist; nlist = tmp;
      if (list_has_match(re, &clist)) { slist_free(&clist); slist_free(&nlist); free(mark); return true; }
    }
  }

out:
  slist_free(&clist);
  slist_free(&nlist);
  free(mark);
  if (exec_limit_exceeded) *exec_limit_exceeded = limit;
  return false;
}
