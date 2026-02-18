# `lines` --- Diamond Builtin Specification (Week 1)

Select and emit specific 1-based input lines by numeric index or range.

------------------------------------------------------------------------

## Status (Week 1)

Implemented and passing full test suite (`make test`).

Load example:

    enable -f ./build/lines.debug.so lines

------------------------------------------------------------------------

## Synopsis

    lines SPEC [--] [FILE...]
    lines --help

------------------------------------------------------------------------

## Diamond Rules Compliance

-   No duplication of full GNU tools
-   Minimal viable feature surface
-   Deterministic behavior
-   Pipeline-first (stdin → stdout)
-   No environment mutation
-   Shared range grammar across builtins
-   1-based indexing
-   Consistent exit codes

------------------------------------------------------------------------

## Exit Codes

  Code   Meaning
  ------ ------------------------------------------------------------------
  0      At least one line emitted
  1      Valid SPEC and readable input, but nothing emitted
  2      Usage error, invalid SPEC, file I/O error, or stdout write error

------------------------------------------------------------------------

## SPEC Grammar

Supported forms:

-   `N`
-   `N,M`
-   `a..b`
-   `..b`
-   `a..`

Whitespace is allowed around `,` and `..`.

------------------------------------------------------------------------

## Normalization Rules

Handled by `dc_sel_parse_and_normalize()`:

-   Sorted ascending
-   Duplicates removed
-   Overlapping ranges merged
-   Reversed ranges invalid
-   Leading zeros invalid
-   Bare `..` invalid
-   Trailing/double commas invalid
-   uint64 overflow invalid

Invalid SPEC ⇒ exit 2.

------------------------------------------------------------------------

## Input Semantics

-   FILEs processed in order.
-   `-` denotes stdin at that position.
-   If no FILEs provided, read stdin.
-   Files concatenated logically.
-   Line numbering continues across files.

------------------------------------------------------------------------

## Line Definition

A line is:

-   A byte sequence ending with `\n`
-   Or a final unterminated sequence at EOF

Newline presence preserved exactly.

------------------------------------------------------------------------

## Output Semantics

-   Matching lines emitted in ascending order.
-   Bytes written exactly as read.
-   No additional newline added.
-   Unterminated final lines remain unterminated.

------------------------------------------------------------------------

## Streaming Behavior

-   Incremental processing.
-   No full-input buffering.
-   Early termination when possible via finite max optimization.

------------------------------------------------------------------------

## Error Handling

Usage errors (exit 2):

-   Missing SPEC
-   Invalid option before `--`
-   Invalid SPEC grammar

Runtime errors (exit 2):

-   File open failure
-   Read error
-   Stdout write failure (closed stdout)

SIGPIPE ignored internally so write failures surface as controlled
runtime errors.

------------------------------------------------------------------------

## Option Parsing Rules

-   Only `--help` recognized.
-   Other `-x` before `--` is usage error.
-   `--` ends option parsing.
-   After `--`, dash-leading filenames allowed.
-   `-` treated as stdin in FILE position.

------------------------------------------------------------------------

## Week 1 Completion Criteria

-   `make test` passes with zero failures.
-   `lines` and `fields` both implemented.
-   Shared range grammar exercised by both.
