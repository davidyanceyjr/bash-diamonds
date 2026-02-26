# `lines`

Select and emit specific **1-based** input lines by numeric index or range.

## Synopsis

    lines SPEC [--] [FILE...]
    lines --help

## Options

- `--help` print help to stdout and exit 0
- `--` end option parsing

## Arguments

- `SPEC` range selection grammar (shared): see `docs/common-range-spec.md`
- `FILE...` optional input files

## Selection semantics

`SPEC` applies to the **concatenation** of all inputs.

- Files are processed left-to-right.
- `-` means stdin at that position.
- If no `FILE` is provided, stdin is used.
- Line numbering continues across files.

## Line model and newline behavior

A “line” is either:

- a byte sequence ending with `\n`, or
- the final unterminated byte sequence at EOF.

Output is written **verbatim** for selected lines:

- newline-terminated lines remain newline-terminated
- an unterminated final line remains unterminated
- no newline is synthesized

## Streaming behavior

Streaming, line-at-a-time. May stop early when the selection has a finite maximum.

## Exit codes

See `docs/common-exit-codes.md`.

## Examples

    # 3rd line
    lines 3 file.txt

    # lines 1 and 3
    lines 1,3 file.txt

    # from line 10 onward
    cmd | lines 10..

    # allow dash-leading filenames
    lines 1 -- -dashfile