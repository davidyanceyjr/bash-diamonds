# `match` — Diamond Builtin Specification (Week 3 Draft)

Filter input lines by a constrained pattern match.

This is NOT GNU `grep`.
This is a minimal, deterministic line filter designed for pipelines.

------------------------------------------------------------------------

## Synopsis

    match PATTERN [--] [FILE...]
    match --help

------------------------------------------------------------------------

## Pattern Model (v1)

PATTERN is one of:

1) Literal substring:
   - default: PATTERN is taken literally
   - case-sensitive
   - byte-oriented

2) Anchors:
   - If PATTERN begins with '^', it matches only at start-of-line.
   - If PATTERN ends with '$', it matches only at end-of-line.
   - Both may be used together.

No regex operators beyond ^ and $ in v1.

------------------------------------------------------------------------

## Output

- Emit the original line bytes exactly as read (including its newline if present).
- If a line matches, it is emitted.
- If it does not match, it is skipped.
- Unterminated final line:
  - If it matches, emit it unterminated (no newline added).

------------------------------------------------------------------------

## Exit Codes

  Code   Meaning
  ------ ----------------------------------------------------
  0      At least one matching line emitted
  1      No matching lines emitted
  2      Usage error, file I/O error, or stdout write error

SIGPIPE must be ignored internally so stdout write failures return 2.

------------------------------------------------------------------------

## Option Parsing

- Only `--help` is recognized.
- Any other `-x` token before `--` is a usage error unless token is exactly '-'.
- `--` ends option parsing.
- `-` in FILE list denotes stdin at that position.

------------------------------------------------------------------------

## Input Semantics

Same concatenation rules as `lines` / `fields` / `trim`.
Reuse dc_lr_* line reader.

------------------------------------------------------------------------

## Non-Goals

- No full regex engine.
- No alternation, capture groups, character classes.
- No invert match, count, filename prefixes, context lines.
