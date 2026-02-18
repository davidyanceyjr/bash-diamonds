# `trim` --- Diamond Builtin Specification (Week 2 Draft)

Remove leading and trailing ASCII whitespace from each input line.

------------------------------------------------------------------------

## Status

Specification draft (pre-implementation anchor). No code committed yet.

------------------------------------------------------------------------

## Synopsis

    trim [--] [FILE...]
    trim --help

------------------------------------------------------------------------

## Diamond Rules Compliance

-   No duplication of full GNU tools
-   Minimal viable feature surface
-   Deterministic behavior
-   Pipeline-first (stdin → stdout)
-   No environment mutation
-   Consistent exit codes
-   Streaming-only implementation

------------------------------------------------------------------------

## Exit Codes

  Code   Meaning
  ------ ----------------------------------------------------
  0      At least one line emitted
  1      Inputs readable but nothing emitted
  2      Usage error, file I/O error, or stdout write error

------------------------------------------------------------------------

## Input Semantics

-   FILEs processed in order.
-   `-` denotes stdin at that position.
-   If no FILEs provided, read stdin.
-   Files concatenated logically.
-   Line numbering not externally visible.

------------------------------------------------------------------------

## Line Definition

A line is:

-   A sequence of bytes ending with `\n`
-   Or a final unterminated sequence at EOF

Newline presence is preserved only if output is emitted for that line.

------------------------------------------------------------------------

## Trimming Rules

For each input line:

1.  Remove leading ASCII whitespace.
2.  Remove trailing ASCII whitespace.
3.  ASCII whitespace includes:
    -   Space (0x20)
    -   Tab (0x09)
    -   Carriage Return (0x0D)
    -   Vertical Tab (0x0B)
    -   Form Feed (0x0C)
    -   Newline (0x0A) is considered structural and not part of the
        content region
4.  Internal whitespace is preserved unchanged.
5.  No Unicode handling.
6.  No configurable delimiter or mode options.

------------------------------------------------------------------------

## Emission Rules

-   If trimmed content length > 0:
    -   Emit trimmed bytes.
    -   If original line ended with `\n`, emit `\n`.
-   If trimmed content length == 0:
    -   Emit nothing for that line.
-   If final line is unterminated and emits content:
    -   Do not append a newline.

------------------------------------------------------------------------

## Streaming Behavior

-   Process input incrementally.
-   No full-input buffering.
-   Only current line retained in memory.
-   No environment modification.

------------------------------------------------------------------------

## Error Handling

Usage errors (exit 2):

-   Unknown `-x` option before `--`.
-   Invalid option syntax.

Runtime errors (exit 2):

-   File open failure.
-   Read error.
-   Stdout write failure (including closed stdout).

SIGPIPE must be ignored internally so write failures produce
deterministic exit 2.

------------------------------------------------------------------------

## Option Parsing Rules

-   Only `--help` is recognized.
-   Any other `-x` before `--` is a usage error.
-   `--` ends option parsing.
-   After `--`, dash-leading filenames are allowed.
-   Token `-` is treated as stdin when in FILE position.

------------------------------------------------------------------------

## Examples

Input:

       alpha  
    \tbeta\t
    gamma
         

Output:

    alpha
    beta
    gamma

------------------------------------------------------------------------

## Non-Goals

-   No pattern-based trimming.
-   No left-only or right-only modes.
-   No custom delimiter support.
-   No environment mutation.
-   No Unicode whitespace handling.

------------------------------------------------------------------------

## Week 2 Acceptance Criteria

-   `make test` passes with zero failures.
-   Deterministic newline semantics verified.
-   Deterministic exit codes verified.
-   Compatible with existing shared line reader.
