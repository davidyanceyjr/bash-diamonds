# `take`

Emit a forward-only slice of input lines.

## Synopsis

    take N [S] [--] [FILE...]
    take --help

Primary form is positional: `take N [S] [--] [FILE...]`.

## Options

- `--help` print help to stdout and exit 0
- `--` end option parsing
- unknown option before `--` is a usage error and exits `2`
- use `--` before dash-leading file operands

## Arguments

- `N` number of lines to emit
- `S` number of lines to skip (default 0)
- `FILE...` optional input files

`N` and `S` are strict base-10 uint64:

- digits only, no sign
- no leading zeros unless exactly `0`
- overflow is an error

## Selection semantics

- `take N` emits lines 1..N
- `take N S` emits lines (S+1)..(S+N)

Stops reading once `N` lines have been emitted.

## Line model and newline behavior

Lines are emitted verbatim (newline preserved if present). No newline is synthesized.

## Exit codes

See `docs/common-exit-codes.md`.

## Examples

    take 10 file.txt

    cmd | take 5 1

    take 20 10 -- a.txt - b.txt

    take 0 file.txt
