# replace

Streaming substitution primitive.

`replace` reads input line-by-line and emits only those lines in which at least one substitution occurred.
This preserves the suite-wide exit-code contract where exit `1` means “no output”.

## Synopsis

    replace [--literal] PATTERN REPLACEMENT [--] [FILE...]
    replace --help

## Modes

### Default mode (regex)
- `PATTERN` is a POSIX Extended Regular Expression (ERE).
- Matching is performed per-line.
- Replaces all non-overlapping matches within a line.

### Literal mode
- `--literal`
- `PATTERN` is treated as an exact byte sequence (no regex interpretation).
- Matching is byte-wise.

## Replacement semantics
- `REPLACEMENT` is a literal byte sequence.
- No escape processing.
- No backreferences in v1.

## Output behavior
Per input line:
- If the line contains one or more matches:
  - emit the substituted line
  - preserve whether the input line ended in `'\n'`
- If the line contains no matches:
  - emit nothing for that line

No trailing whitespace/delimiters are added. Byte content is preserved except for the performed substitutions.

## Streaming behavior
- Pure streaming, line-at-a-time.
- Only the current line may be buffered.
- Multi-file input is processed left-to-right.

## Exit codes
- `0` if at least one line was emitted (i.e., at least one replacement occurred)
- `1` if no replacements occurred in the entire run (no output)
- `2` on:
  - usage error
  - invalid regex
  - I/O error (including stdout write failure)

## Strict option parsing
- `--help` prints help to stdout and exits 0.
- `--` ends option parsing.
- Unknown `-x` before `--` (other than literal `-`) exits 2.
- `--literal` must appear before `PATTERN`.
- Duplicate `--literal` is a usage error (exit 2).

## Edge cases
### Empty PATTERN
Usage error, exit 2.

### Zero-length regex matches
Must not infinite-loop. Implementations must advance at least one byte after a zero-length match.

### Binary input
Allowed. Operates on raw bytes with the suite line model.

## Examples

Replace digits with `X` (regex mode), emitting only matching lines:

    replace '[0-9]+' X file.txt

Literal replacement:

    replace --literal foo bar file.txt

Multiple files:

    replace error ERROR log1 log2

End option parsing (filename begins with '-'):

    replace --literal foo bar -- -input.txt