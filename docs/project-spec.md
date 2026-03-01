```markdown
# Bash Diamonds — Project Specification (Normative)

This document defines the **developer-side normative contract** for the Bash Diamonds suite.

It governs:

- Suite-wide invariants
- Exit semantics
- Input/output guarantees
- Diagnostic behavior
- Builtin classification and stability
- Conformance expectations

If any other documentation conflicts with this document, this document prevails.

User-facing guidance, composition examples, and philosophy are defined in `docs/diamond-suite.md`.

---

## 1. Document Roles and Authority

### 1.1 Normative Contract

`project-spec.md` is the authoritative suite contract.

It defines cross-suite invariants and stability guarantees.

### 1.2 Builtin Specifications

Each builtin has its own specification document.

That document:

- Defines complete behavior for that builtin.
- May extend suite invariants.
- Must not contradict suite invariants.
- Defines exit code 1 semantics if applicable.

### 1.3 Handbook (Non-Normative)

`diamond-suite.md`:

- Is user-facing.
- Describes purpose, concepts, and composition patterns.
- Contains executable examples.
- Is not normative.

If handbook examples conflict with this document, the handbook is incorrect.

---

## 2. Suite-Wide Behavioral Invariants

The following invariants apply to all builtins.

---

### 2.1 Streaming Model

- Builtins operate in a streaming, line-oriented manner.
- Tools must not buffer entire input unless explicitly documented.
- Input is processed sequentially.
- No implicit sorting or reordering is allowed unless explicitly specified.

---

### 2.2 Line Model and Newline Preservation

- Input is treated as raw byte streams segmented by `\n`.
- If the final input line is not newline-terminated, it must still be processed.
- If a builtin emits an input line unchanged or derived directly from it, it must preserve whether that line ended with `\n`.
- Builtins must not synthesize or strip trailing newlines unless explicitly defined in their specification.

This newline preservation rule is part of the contract.

---

### 2.3 Byte Model and Locale Independence

- Tools operate on raw bytes.
- No implicit UTF-8 or Unicode semantics are applied.
- No locale-dependent behavior is permitted.
- All comparisons and processing are byte-based unless explicitly documented.

---

### 2.4 Input Sources

- If no file arguments are provided, input is read from standard input.
- A literal `-` represents standard input.
- Multiple files are processed strictly in the order provided.
- File open failure must:
  - Emit a diagnostic to stderr.
  - Exit with status 2.
- Fatal I/O errors must terminate processing.

---

### 2.5 Option Parsing and Help Behavior

All builtins must conform to the following:

- `--help`:
  - Prints help text to stdout.
  - Exits with status 0.
  - Performs no further processing.
- Unknown options:
  - Emit a diagnostic to stderr.
  - Exit with status 2.
- `--` terminates option parsing.
- Options must not be accepted after positional arguments unless explicitly documented.

The exact wording of help text is defined per builtin.

---

### 2.6 Command-Line Interface Conventions (Positional Defaults)

The Bash Diamonds suite prioritizes readable pipelines. Each builtin must support a positional-argument invocation that covers the common use case.

#### 2.6.1 80/20 Rule

For each builtin:

- The most common operation (the “80% use case”) must be expressible **without flags** using positional arguments.
- Flags exist to refine or modify behavior (the “20% use case”).
- A builtin must not require a flag to specify its primary action.

This is a suite-wide contract requirement.

#### 2.6.2 Canonical Positional Form

Unless explicitly documented otherwise in the builtin specification, the canonical invocation form is:

```

<tool> [PRIMARY] [FILES...]

```

Where:

- `PRIMARY` is the primary selector/argument for the tool (e.g., a field selector, a pattern, a count).
- `FILES...` are optional input sources; if omitted, stdin is used.

#### 2.6.3 Compatibility of Flags

If a builtin previously required flags for its primary action (e.g., `-f`, `-m`, `-n`):

- The flagged form may remain supported as an alias for compatibility.
- The positional form is the preferred interface and must be documented as such.
- If both positional and flagged forms are present and conflict, the builtin specification must define precedence deterministically.

#### 2.6.4 `--help` Must Document Positional Defaults First

Help output must present the positional form as the primary usage line, followed by optional flags and extended forms.

Example pattern:

```

Usage: fields <selector> [FILE...]
...
Options:
...

```

---

### 2.7 Exit Code Semantics (Preserved)

All builtins must use only the following exit codes:

- **0** — successful execution with output produced.
- **1** — successful execution with no output produced.
- **2** — usage error, runtime error, I/O error, or write failure.

Write failures to stdout (including broken pipe conditions) must result in exit code 2.

No builtin may return any other exit code.

---

### 2.8 Stdout and Write Failure Handling

- Builtins must detect write failures to stdout.
- Write failure must result in exit code 2.
- A builtin must not return success (0 or 1) if output flushing fails.

---

### 2.9 Diagnostics

- Diagnostics must be written exclusively to stderr.
- Normal output must never be written to stderr.
- Diagnostics must be deterministic.
- File-related errors must identify the file.
- Usage errors must clearly describe misuse.

Diagnostic prefix format may be defined per builtin but must remain consistent within that builtin.

---

## 3. Builtin Inventory and Stability

This section defines the authoritative suite inventory.

### 3.1 Stable (Part of v1.0 Contract)

The following builtins are considered stable:

- lines
- fields
- match
- take
- trim
- table
- count
- filter

Stable builtins guarantee:

- Option names and meanings are stable (including positional defaults defined in §2.6).
- Output format is stable.
- Exit semantics are stable.
- Documented behavior will not change without versioning.

---

### 3.2 Preview

- replace  *(if applicable)*

Preview builtins are functional but may evolve before stabilization.

---

### 3.3 Planned

- freq
- alone
- arrange

Planned builtins are not part of the contract.

---

## 4. Development Iteration Model

Development follows this loop:

1. Define or update specification.
2. Implement behavior.
3. Add or update tests.
4. Update builtin documentation.
5. Update handbook examples if relevant.
6. Merge feature branch.

Behavior must not exceed specification.

Specification must not promise behavior not implemented.

If a change affects command-line surface area, it must maintain the positional-default conventions in §2.6.

---

## 5. Conformance

The test suite validates:

- Suite-wide invariants.
- Builtin-specific behavior.
- Exit semantics.
- Newline preservation.
- Deterministic diagnostics.
- Stdout write failure handling.
- Positional-default behavior for common use cases (where applicable), and any documented compatibility forms.

Tests validate contract adherence.  
They do not imply performance or production representativeness.

---

## 6. Contract Changes

Changes to:

- Suite invariants
- CLI positional-default conventions
- Exit semantics
- Stability classification
- Byte model
- Newline preservation rules

constitute contract-level modifications and must be explicitly updated in this document.

---

# End of Project Specification
```
