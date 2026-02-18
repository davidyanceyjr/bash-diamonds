# `fields` --- Diamond Builtin Specification (Week 1)

Select and emit specific 1-based whitespace-delimited fields from each
input line.

------------------------------------------------------------------------

## Status (Week 1)

Implemented and passing full test suite (`make test`).

Load example:

    enable -f ./build/fields.debug.so fields

------------------------------------------------------------------------

## Synopsis

    fields SPEC [--] [FILE...]
    fields --help

------------------------------------------------------------------------

## Diamond Rules Compliance

-   No duplication of full GNU tools
-   Minimal feature surface
-   Deterministic behavior
-   Pipeline-first (stdin → stdout)
-   No environment mutation
-   Shared range grammar
-   1-based indexing
-   Consistent exit codes

------------------------------------------------------------------------

## Exit Codes

  Code   Meaning
  ------ ------------------------------------------------------------------
  0      At least one field emitted
  1      Valid SPEC and readable input, but nothing emitted
  2      Usage error, invalid SPEC, file I/O error, or stdout write error

------------------------------------------------------------------------

## SPEC Grammar

Identical to `lines`:

-   `N`
-   `N,M`
-   `a..b`
-   `..b`
-   `a..`

Whitespace allowed around separators.

Normalization identical to `lines` via shared parser.

------------------------------------------------------------------------

## Input Semantics

-   FILEs processed in order.
-   `-` denotes stdin at that position.
-   If no FILEs provided, read stdin.
-   Files concatenated logically.
-   Selection applied per line (not global indexing).

------------------------------------------------------------------------

## Field Splitting Rules

For each line:

-   Split on ASCII whitespace:
    -   space, tab, newline, carriage return, vertical tab, form feed
-   Collapse runs of whitespace.
-   Ignore leading/trailing whitespace.
-   No empty fields.
-   No quoting or escaping.
-   No custom delimiter support.

------------------------------------------------------------------------

## Selection Semantics

-   Selection applied per line (1-based).
-   Fields emitted in ascending order.
-   Duplicates removed via normalized selection.
-   Selected fields joined by a single ASCII space.
-   If no fields selected on a line, emit nothing for that line.
-   Newline preserved if original line ended with newline and output
    emitted.

------------------------------------------------------------------------

## Streaming Behavior

-   Processes input incrementally.
-   Only current line buffered.
-   Field views reference original line buffer (no field byte copying).

------------------------------------------------------------------------

## Error Handling

Usage errors (exit 2):

-   Missing SPEC
-   Invalid option before `--`
-   Invalid SPEC grammar

Runtime errors (exit 2):

-   File open failure
-   Read error
-   Stdout write failure

SIGPIPE ignored internally to ensure deterministic runtime error
reporting.

------------------------------------------------------------------------

## Week 1 Completion Criteria

-   `make test` passes with zero failures.
-   Shared range grammar exercised.
-   Deterministic newline and exit semantics validated.
