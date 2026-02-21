# take — Forward Line Slice (Diamond Builtin)

## Name

take — emit a forward-only slice of input lines

## Synopsis

take N [S] [--] [FILE...]
take --help

## Description

take emits a contiguous forward slice of input lines.

- take N emits the first N lines.
- take N S skips S lines, then emits the next N lines.

take is streaming and may terminate early once N lines have been emitted.

There is no negative indexing and no end-relative addressing.

Input is read from FILE... left-to-right. If no files are given, input is read from standard input. A filename of "-" means standard input at that position.

Line numbering is 1-based across the concatenation of all inputs.

---

## Arguments

### N (required)

Unsigned base-10 integer (0 allowed).

Number of lines to emit.

### S (optional)

Unsigned base-10 integer (0 allowed).

Number of lines to skip before emitting.

If omitted, S = 0.

### FILE... (optional)

Zero or more input files.

- "-" represents standard input.
- If omitted, input is standard input.

---

## Selection Semantics

Let input lines be numbered 1, 2, 3, … across all inputs.

- take N emits lines 1 through N.
- take N S emits lines (S + 1) through (S + N).

If fewer than S + N lines exist, take emits whatever lines are available after skipping S.

If no lines qualify for output, take produces no output.

---

## Line Model

A line is:

- a sequence of bytes ending with '\n', or
- a final unterminated byte sequence at EOF.

take preserves input exactly:

- newline-terminated lines remain newline-terminated
- unterminated final lines remain unterminated
- no newline synthesis is performed

---

## Streaming Behavior

take processes input incrementally.

- After emitting N lines, it stops reading further input.
- No full-input buffering is performed.
- Memory usage is O(1) with respect to input size.

SIGPIPE is ignored internally. A write failure on standard output is treated as a runtime error.

---

## Numeric Parsing Rules

N and S must:

- consist only of ASCII digits (0-9)
- contain no sign ('+' or '-' invalid)
- contain no leading zeros unless the value is exactly "0"
  - "0" is valid
  - "00", "01" are invalid
- fit within uint64_t (overflow is an error)

Invalid numeric arguments result in exit status 2.

---

## Option Handling

Recognized options:

- --help
- --

Any other token beginning with '-' before -- is a usage error.

After --, remaining tokens are treated as positional arguments (including filenames beginning with '-').

---

## Exit Status

- 0 — one or more lines emitted
- 1 — valid execution, no lines emitted
- 2 — usage error or runtime error

Examples returning 1:

- take 0
- fewer than S + 1 input lines exist
- empty input with N > 0

---

## Errors (Exit 2)

- Missing N
- Invalid N or S (syntax, leading zeros, sign, overflow)
- Unsupported option before --
- Too many positional arguments before files
- File open or read error
- Standard output write error (including EPIPE)

---

## Examples

Emit first 10 lines:

    cmd | take 10

Skip header line, emit next 5:

    cmd | take 5 1

Process multiple files:

    take 20 10 -- a.txt - b.txt

Emit nothing (valid execution):

    take 0 file.txt

---

## Design Constraints

- Forward-only indexing
- No end-relative selection
- No delimiter customization
- No regex or field parsing
- Deterministic behavior
- No environment mutation

take is a streaming, minimal primitive for forward line selection.