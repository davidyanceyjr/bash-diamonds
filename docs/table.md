# `table`

Format whitespace-delimited text into aligned columns for human output.

## Synopsis

    table [--] [FILE...]
    table --help

Primary form is positional: `table [--] [FILE...]`.

## Options

- `--help` print help to stdout and exit 0
- `--` end option parsing
- unknown option before `--` is a usage error and exits `2`
- use `--` before dash-leading file operands

## Arguments

- `FILE...` input files

## Input and output

- Input lines are split into fields by runs of space and tab.
- Leading and trailing space/tab are ignored for field detection.
- Empty or whitespace-only lines are not emitted.
- Output columns are padded with spaces to the maximum width of each column.
- Minimum separation between columns is:
  - 1 space between column 1 and column 2
  - 2 spaces between later columns
- No trailing spaces are written.
- Newlines are preserved only when an input line is emitted.

## Seekability rule

`table` uses a strict two-pass formatter. All inputs must be seekable regular files.

- stdin is not supported (including `-`).
- If stdin or any non-seekable input is used: exit 2 and print:
  `table: non-seekable input not supported`

## Exit codes

See `docs/common-exit-codes.md`.

## Examples

Input:
    a   bb   c
    aaa b    ccc

Output:
    a   bb  c
    aaa b   ccc
