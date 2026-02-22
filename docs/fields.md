# `fields` — Diamond Builtin Specification (v1.0 Draft)

Select and emit specific 1-based fields from each input line.

By default, fields are **ASCII whitespace-delimited**.

With `-d`, fields are **single-byte delimiter-separated** (minimal, cut-like,
streaming, no quoting rules).

------------------------------------------------------------------------

## Synopsis

    fields [-d DELIM] SPEC [--] [FILE...]
    fields --help

Where:

- `SPEC` is the shared 1-based selection grammar (same as `lines`).
- `DELIM` is exactly **one byte** (one character).
- `-d` may be written as `-dX` or `-d X`.

Examples:

    fields 2
    fields 1,3
    fields -d: 1,7 /etc/passwd
    fields -d ':' 1,7
    fields -d'|' 2
    fields -d$'\t' 3

------------------------------------------------------------------------

## Diamond Rules Compliance

- No duplication of full GNU tools
- Minimal feature surface
- Deterministic behavior
- Pipeline-first (stdin → stdout)
- Streaming only (no full-input buffering)
- No environment mutation
- Shared range grammar
- 1-based indexing
- Consistent exit codes

------------------------------------------------------------------------

## Exit Codes

| Code | Meaning |
|------|----------|
| 0 | At least one output line emitted |
| 1 | Valid SPEC and readable input, but nothing emitted |
| 2 | Usage error, invalid SPEC, invalid DELIM, file I/O error, or stdout write error |

“Output line emitted” means `fields` produced output for at least one input line
(even if the emitted line is empty bytes due to selecting an empty field).

------------------------------------------------------------------------

## SPEC Grammar

Identical to `lines`:

- `N`
- `N,M`
- `a..b`
- `..b`
- `a..`

Whitespace allowed around separators.

Normalization identical to `lines` via shared parser.

Selection is per line (not global across input).

------------------------------------------------------------------------

## Option Parsing

Recognized options:

- `--help` → print usage to stdout, exit 0.
- `--` → end option parsing; remaining tokens are FILE names.
- `-d DELIM` or `-dDELIM` → enable delimiter mode.

Rules:

- `-d` requires exactly one byte after shell expansion.
- `-dX` → delimiter is `X`.
- `-d X` → delimiter is `X`.
- If the resolved `DELIM` length is not exactly 1 byte → usage error (exit 2).
- Any other `-token` before `--` → usage error (exit 2).

------------------------------------------------------------------------

## Input Semantics

- FILEs processed in order.
- `-` denotes stdin at that position.
- If no FILEs provided → read stdin.
- Files are concatenated logically.
- Processing is line-by-line (streaming).
- Selection applies independently per line.

------------------------------------------------------------------------

## Field Splitting Modes

### 1) Default Mode (Whitespace)

- Split on ASCII whitespace:
  - space, tab, newline, carriage return, vertical tab, form feed
- Collapse runs of whitespace.
- Ignore leading/trailing whitespace.
- No empty fields.
- No quoting or escaping.
- Trailing newline not part of any field.

This matches current v1 behavior.

---

### 2) Delimiter Mode (`-d`)

- Split on the single-byte delimiter `DELIM`.
- Do **not collapse** consecutive delimiters.
- **Preserve empty fields** between delimiters.
- Leading delimiter → leading empty field.
- Trailing delimiter → trailing empty field.
- No quoting, escaping, CSV handling, or multi-byte delimiters.
- Trailing newline (if present) is not part of any field.

Example (`-d:`):

Input:

    a::c

Fields:

    1 = "a"
    2 = ""
    3 = "c"

------------------------------------------------------------------------

## Selection & Output Semantics

- 1-based indexing.
- Fields emitted in ascending order.
- Duplicate selections removed (via normalized SPEC).
- Selected fields are joined with a single ASCII space (`0x20`).
- This joining rule applies in both whitespace and delimiter mode.

Per-line emission rules:

- If no selected field positions exist → emit nothing for that line.
- If selected positions exist but all selected fields are empty →
  emit an empty output line (zero bytes), respecting newline rules.

------------------------------------------------------------------------

## Newline Preservation

For each input line:

- If an output line is emitted:
  - If input ended with newline → output ends with newline.
  - If input did not end with newline → output does not end with newline.
- If no output emitted → emit nothing (no newline).

No newline is synthesized.

------------------------------------------------------------------------

## Streaming Guarantees

- Only the current input line is buffered.
- Field views reference the original line buffer (no field byte copying).
- No full-input buffering.

------------------------------------------------------------------------

## Error Handling

Usage errors (exit 2):

- Missing SPEC
- Invalid SPEC grammar
- Unknown option before `--`
- `-d` missing argument
- `DELIM` length != 1 byte

Runtime errors (exit 2):

- File open failure
- Read error
- Stdout write failure

SIGPIPE is ignored internally to ensure deterministic runtime error reporting.

------------------------------------------------------------------------

## Explicit Non-Goals

- No multi-byte delimiters
- No regex delimiters
- No CSV quoting/escaping
- No output delimiter customization
- No global field indexing across files

------------------------------------------------------------------------

## Completion Criteria (v1.0)

- Whitespace mode behavior unchanged and fully tested.
- `-d` supports single-byte delimiters only.
- Empty-field behavior deterministic and tested.
- `/etc/passwd | fields -d: 1,7` works reliably.
- All tests pass: `make test` → zero failures.