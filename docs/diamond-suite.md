# Bash Diamonds — Suite Specification

This document defines the shared, suite-wide behavioral contract for all Bash Diamonds builtins.

## Implemented builtins (current)

- lines
- fields
- match
- take
- trim
- table
- count

Future-week stubs may exist in the tree but are not considered part of the implemented suite unless explicitly listed above.

## Global invariants (apply to all implemented builtins)

### Streaming
- All tools must operate in a pure streaming model: process input line-by-line.
- No full-input buffering.
- Input is read using the shared dc_lr_* line reader.

### Line model and newline determinism
- A “line” is either:
  - a maximal byte sequence ending in '\n', or
  - the final unterminated byte sequence at EOF.
- No tool may synthesize or strip a trailing newline unless its spec explicitly says so.
- Bytes are not interpreted (binary-safe); tools operate on raw bytes.

### Input files / stdin
- If no FILE arguments are provided, read from stdin.
- A FILE of '-' means stdin at that position.
- '-' may appear among other files and must be processed in order.
- Filenames beginning with '-' must be preceded by '--'.

### Option parsing
- '--help' prints help to stdout and exits 0.
- '--' ends option parsing.
- Strict parsing: any unknown '-x' token before '--' (other than literal '-') is a usage error and must exit 2.
- No environment mutation.

### Exit codes (suite-wide)
- 0 = success with output
- 1 = valid execution, no result
- 2 = usage error or runtime error

Exception:
- count never exits 1; it is 0 on success (including output "0") and 2 on error.

### SIGPIPE and stdout write failures
- SIGPIPE must be ignored.
- Any stdout write failure (including closed pipe) is a controlled runtime error and must exit 2.
- Tools must force detection of stdout failure (e.g., via flush or equivalent) before returning success.

### Error output
- On usage/runtime error, diagnostics go to stderr only.
- Help and normal output go to stdout only.
- Error messages must be deterministic (no nondeterministic paths or context-dependent output).