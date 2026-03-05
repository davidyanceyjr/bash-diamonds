# `trim`

Remove leading and trailing ASCII whitespace from each input line.

Lines that become empty after trimming are not emitted.

## Synopsis

    trim [--] [FILE...]
    trim --help

Primary form is positional: `trim [--] [FILE...]`.

## Options

- `--help` print help to stdout and exit 0
- `--` end option parsing
- unknown option before `--` is a usage error and exits `2`
- use `--` before dash-leading file operands

## Arguments

- `FILE...` optional input files

## Input and output

- Files are processed left-to-right; `-` means stdin.
- If no `FILE` is provided, stdin is used.
- Trims ASCII: space, tab, CR, VT, FF.
- Newline is structural and preserved only when a line is emitted.
- No newline is synthesized.

## Exit codes

See `docs/common-exit-codes.md`.

## Examples

    printf '  a  \n' | trim

    trim file.txt

    cmd | trim | match '^x'
