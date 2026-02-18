# lines

## Synopsis

    lines SPEC [--] [FILE...]
    lines --help

Select and emit specific 1-based input lines by numeric index or range.

------------------------------------------------------------------------

## Description

`lines` reads from stdin and/or one or more files and emits only the
lines selected by `SPEC`.

If multiple files are provided, they are logically concatenated into a
single continuous input stream. Line numbering is global across all
inputs.

`-` may be used within `FILE...` to represent stdin.

Line numbering is **1-based**.

------------------------------------------------------------------------

## Range Specification

### Valid Forms

-   `N` --- select a single line
-   `N,M,K` --- select multiple explicit lines
-   `A..B` --- closed range (inclusive)
-   `..B` --- lines `1` through `B`
-   `A..` --- lines `A` through end-of-input

Whitespace is allowed around `,` and `..`.

### Examples

    lines 1
    lines 2,4,9
    lines 3..7
    lines ..5
    lines 10..

### Rules

-   Indices must be positive integers (1-based).
-   Zero and negative values are invalid.
-   Leading zeros are invalid (`01` is an error).
-   Reversed ranges (`5..2`) are invalid.
-   Duplicate indices are emitted once.
-   Overlapping ranges are merged.
-   Selection is emitted in ascending numeric order.
-   `..` alone is invalid.

------------------------------------------------------------------------

## Input Handling

-   If no files are specified, input is read from stdin.
-   If files are specified, they are processed in the order given.
-   `-` represents stdin and may appear multiple times.
-   Line numbering continues across file boundaries.

------------------------------------------------------------------------

## Line Semantics

A line is:

-   A sequence of bytes ending with `\n`, OR
-   The final sequence of bytes at EOF if no trailing newline exists.

Output preserves original bytes exactly, including newline presence or
absence.

`lines` never adds or removes newline characters.

------------------------------------------------------------------------

## Exit Codes

| Code \| Meaning \|

\|------\|---------\| 0 \| Success and at least one line emitted \| \| 1
\| Valid SPEC but no selected lines exist \| \| 2 \| Usage error or
runtime error \|

Errors include:

-   Invalid `SPEC`
-   Missing `SPEC`
-   File open/read failure
-   Output write failure

------------------------------------------------------------------------

## Examples

### Basic

    printf "a\nb\nc\n" | lines 2
    # b

    printf "a\nb\nc\n" | lines 1,3
    # a
    # c

### Ranges

    printf "a\nb\nc\n" | lines 2..
    # b
    # c

    printf "a\nb\nc\n" | lines ..2
    # a
    # b

### Multiple Files

    lines 3 file1 file2

If `file1` has two lines and `file2` begins with `c`, the result is `c`.

### stdin in the middle

    lines 2 file1 - file2

Line numbering continues across all sources.

------------------------------------------------------------------------

## Common Pipeline Mappings

| Traditional \| Diamond \|

\|-------------\|----------\| sed -n 'Np' \| lines N \| \| head -n K \|
lines ..K \| \| tail -n +N \| lines N.. \| \| head -n K \| tail -n 1 \|
lines K \|

------------------------------------------------------------------------

## Non-Goals

`lines` does NOT:

-   Print line numbers
-   Support negative indexing
-   Support regex selection
-   Perform sorting or reordering beyond normalization
-   Emulate full `sed`, `head`, or `tail`
-   Modify environment or shell state
-   Perform locale-aware behavior

------------------------------------------------------------------------

## Determinism

For identical byte input and identical `SPEC`, output is byte-identical.

No locale, environment, or time-dependent behavior.
