# `match`

Filter input lines by a deterministic, constrained regex.

This is not GNU grep.

## Synopsis

    match PATTERN [--] [FILE...]
    match --help

## Options

- `--help` print help to stdout and exit 0
- `--` end option parsing

## Arguments

- `PATTERN` constrained regex pattern
- `FILE...` optional input files

## Input and output

- Files are processed left-to-right; `-` means stdin.
- If no `FILE` is provided, stdin is used.
- Matching is evaluated on the line **excluding** a terminating `\n`.
- If matched, the original line bytes are emitted verbatim (newline preserved if present).
- No newline is synthesized.

## Pattern features (subset)

Supported operators:

- literals
- `.` any single byte
- classes `[...]` (with ranges like `a-z`)
- grouping `( ... )`
- alternation `|`
- quantifiers `* + ?`
- anchors: `^` at start, `$` at end

The engine is intentionally constrained; invalid constructs are compile errors.

## Exit codes

See `docs/common-exit-codes.md`.

Additional exit-2 cases:

- pattern compile error
- regex execution limit exceeded

## Examples

    match 'foo' file.txt

    cmd | match '^ERROR:'

    # allow dash-leading filenames
    match '[0-9]+' -- -dashfile