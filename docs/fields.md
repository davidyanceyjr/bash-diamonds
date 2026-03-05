# `fields`

Select and emit specific **1-based** fields from each input line.

Default splitting is **ASCII whitespace**.
With `-d`, splitting is **single-byte delimiter** based.

## Synopsis

    fields [--tsv] [-d DELIM] SPEC [--] [FILE...]
    fields --help

Primary form is positional: `fields [--tsv] [-d DELIM] SPEC [--] [FILE...]`.

## Options

- `--help` print help to stdout and exit 0
- `--` end option parsing
- unknown option before `--` is a usage error and exits `2`
- use `--` before dash-leading file operands
- `--tsv` join selected fields with a single literal TAB byte (`0x09`) instead of a space
- `-d DELIM` delimiter mode (`-dX` or `-d X`)
  - `DELIM` must be exactly **1 byte** after shell expansion
  - duplicate `-d` is a usage error

## Arguments

- `SPEC` range selection grammar (shared): see `docs/common-range-spec.md`
- `FILE...` optional input files

## Input semantics

- Files are processed left-to-right.
- `-` means stdin at that position.
- If no `FILE` is provided, stdin is used.
- Selection applies **independently to each input line**.

## Field splitting

### Whitespace mode (default)

- Delimiters: ASCII whitespace
- Runs collapse
- Leading/trailing whitespace ignored
- No empty fields

### Delimiter mode (`-d`)

- Split on the single byte `DELIM`
- Empty fields are preserved (consecutive/leading/trailing delimiters)

## Output and newline behavior

- Selected fields are emitted in ascending order.
- Selected fields are joined with:
  - a single ASCII space (`0x20`) by default
  - a single TAB byte (`0x09`) with `--tsv`
- No trailing join delimiter.
- A line is emitted only if at least one selected field position exists for that line.
- If a line is emitted:
  - input newline is preserved
  - no newline is synthesized

## Streaming behavior

Streaming, line-at-a-time. Only the current line is buffered.

## Exit codes

See `docs/common-exit-codes.md`.

## Examples

    # 2nd whitespace field
    fields 2 file.txt

    # 1st and 3rd whitespace fields
    fields 1,3 file.txt

    # TSV join
    fields --tsv 1,3 file.txt

    # passwd username + shell (colon-delimited)
    fields -d: 1,7 /etc/passwd

    # preserve empty fields in -d mode
    printf 'a::c\n' | fields -d: 1..3
