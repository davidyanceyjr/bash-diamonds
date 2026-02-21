# `table` — Diamond Builtin Specification (Week 3 Draft)

Format delimited text into aligned columns for human output.

This is NOT a clone of GNU `column`.
This is a minimal, deterministic formatter for interactive use.

------------------------------------------------------------------------

## Synopsis

    table [--] [FILE...]
    table --help

No other options in draft v1.

------------------------------------------------------------------------

## Exit Codes

  Code   Meaning
  ------ ----------------------------------------------------
  0      At least one output line emitted
  1      Valid input but no output lines emitted
  2      Usage error, file I/O error, or stdout write error

SIGPIPE must be ignored internally so stdout write failures return 2.

------------------------------------------------------------------------

## Input Semantics

- FILEs processed in order.
- `-` denotes stdin at that position.
- If no FILEs provided, read stdin.
- Input is line-based using shared dc_lr_* line reader.

------------------------------------------------------------------------

## Parsing Model (v1)

- Each input line is split into fields by runs of ASCII whitespace:
  - space (0x20) and tab (0x09) only.
- Leading/trailing whitespace ignored for field detection.
- Empty/whitespace-only lines produce no output.

This matches the shared “field model” used by `fields`.

------------------------------------------------------------------------

## Formatting Behavior

- Compute the maximum width of each column across all *emitted* lines.
- Emit each line as:
  - field1 + padding + field2 + padding + ... + fieldN
- Padding is:
  - at least one space between columns
  - plus additional spaces to align columns to max widths
- No trailing spaces at end of output line.
- Newline preservation:
  - If the original line ended with '\n', output ends with '\n'
  - If final line is unterminated, do not append '\n'

------------------------------------------------------------------------

## Streaming Constraint vs Column Widths

`table` requires column widths, which normally implies buffering.
To remain streaming-only, v1 uses a strict two-pass rule:

- If input is seekable regular files: perform a first pass to compute widths,
  then rewind and emit formatted output (still no full buffering).
- If stdin is used (or any non-seekable stream): usage/runtime error (exit 2)
  with message: "table: non-seekable input not supported".

Rationale: deterministic formatting without full buffering.

------------------------------------------------------------------------

## Non-Goals

- No custom delimiter options.
- No right/left alignment options.
- No truncation, wrapping, colors, headers.
- No Unicode width handling.
- No buffering entire stdin.

------------------------------------------------------------------------

## Examples

Input:
    a   bb   c
    aaa b    ccc

Output:
    a   bb  c
    aaa b   ccc
