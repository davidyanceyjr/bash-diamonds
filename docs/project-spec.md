# Bash Diamonds — Project Specification (Normative)

This document is the single normative contract for the Bash Diamonds suite.

It defines:
- Suite-wide invariants
- CLI and exit semantics
- Builtin stability status
- Governance and change discipline
- Conformance expectations

If any other repository document conflicts with this file, this file prevails.

## 1. Document Roles and Authority

### 1.1 Normative Contract

`docs/project-spec.md` is the only normative suite contract.

### 1.2 User Handbook (Non-Normative)

`docs/diamond-suite.md` is the user-facing handbook.

It may explain workflows and provide examples, but it does not define contract behavior.

### 1.3 Other Docs

Other files under `docs/` are implementation notes and references.

They are not authoritative for behavior. During migration, they may lag behind this contract.

## 2. Suite-Wide Behavioral Invariants

The following invariants apply to all builtins unless this document states otherwise.

### 2.1 Streaming Model

- Builtins process input as a stream.
- Builtins must not buffer full input unless explicitly required by this contract.
- Processing order is sequential and stable.
- No implicit reordering unless explicitly specified.

### 2.2 Line and Newline Model

- Input is raw bytes segmented by `\n`.
- Unterminated final lines must be processed.
- Builtins emitting original or derived lines must preserve newline termination state.
- Builtins must not invent trailing newlines unless explicitly defined.

### 2.3 Byte Model

- Processing is byte-oriented, not Unicode/locale semantic.
- No implicit normalization.
- Comparisons are byte-based unless explicitly stated.

### 2.4 Input Sources

- With no files, input is stdin.
- `-` denotes stdin at that position.
- Multiple inputs are processed in given order.
- File open/read failures must diagnose to stderr and exit `2`.

### 2.5 Option Parsing

- `--help` prints help to stdout and exits `0`.
- Unknown option before `--` is a usage error and exits `2`.
- `--` terminates option parsing.
- Options are not accepted after positionals unless explicitly documented here.

### 2.6 Positional-Primary CLI

- Each builtin's primary action must be available via positional arguments.
- Flags may refine behavior.
- Positional form is the preferred interface.

### 2.7 Exit Codes

Only these exit codes are allowed:

- `0`: success, output emitted
- `1`: success, no output emitted
- `2`: usage/runtime/I/O/write failure

No other exit codes are permitted.

### 2.8 Stdout Failure Handling

- Stdout write failures must produce exit `2`.
- Builtins must detect terminal stdout failure.
- Builtins must not return `0`/`1` if final `fflush(stdout)` or `ferror(stdout)` indicates failure.

### 2.9 Diagnostics

- Diagnostics go to stderr only.
- Normal output goes to stdout only.
- Diagnostics must be deterministic and actionable.

## 3. Builtin Inventory and Stability

### 3.1 Stable (v1.0 Contract)

- lines
- fields
- match
- take
- trim
- table
- count
- filter
- replace

Stable builtins guarantee CLI/output/exit compatibility unless versioned.

### 3.2 Preview

None.

Preview builtins are functional but may evolve before stabilization.

### 3.3 Planned

None.

### 3.4 Implemented but Non-Contract

None.

## 4. Governance and Change Discipline

### 4.1 Required Update Rule

Any contract-affecting change must update this file and tests in the same change set.

Contract-affecting includes changes to:
- Streaming/newline/byte semantics
- CLI parsing or positional interface
- Exit code behavior
- Stability classification

### 4.2 Handbook Sync Rule

If user-facing behavior or examples change, update `docs/diamond-suite.md` in the same PR.

### 4.3 Merge Gate

A merge to `main` must satisfy:
- Contract/docs are internally consistent
- Tests pass
- Stability inventory is accurate

## 5. Conformance

The test suite is the executable conformance check for this contract.

Tests validate behavior and invariants; they do not assert production performance representativeness.
