# AGENTS.md --- Bash Diamonds (Codex Agent Instructions)

This repository contains **Bash loadable builtins** (C) designed for
**streaming pipelines**.

Agents working in this repository must prioritize **small, safe,
contract-preserving patches** over large refactors.

------------------------------------------------------------------------

# Source of truth

Read these first and treat them as normative contracts:

-   `docs/project-spec.md` (suite-wide invariants and exit-code model)
-   `docs/development-governance.md` (workflow and doc roles)
-   `docs/<tool>.md` (per-builtin spec; one file per tool)

If documentation and code disagree:

1.  **Do not change behavior silently**
2.  Identify the exact spec section involved
3.  Report the mismatch clearly
4.  Propose the smallest fix that restores compliance to the
    specification
5.  Update tests to lock the contract

------------------------------------------------------------------------

# Repository map

-   Builtins: `src/builtins/builtin_<name>.c`
-   Shared core: `src/diamondcore/*.c`
-   Shared headers: `src/include/**` and `src/builtins/**`
-   Tests: `tests/*.bats`
-   Docs/specs: `docs/`

------------------------------------------------------------------------

# Repository navigation hints

When investigating behavior, scan files in this order:

1.  `docs/project-spec.md`
2.  `docs/<tool>.md`
3.  `src/builtins/builtin_<tool>.c`
4.  `src/diamondcore/*.c` (if referenced)
5.  `tests/*.bats`

Avoid scanning unrelated directories unless necessary.

------------------------------------------------------------------------

# Build and test

### Build (debug)

``` sh
make debug
```

### Build (release)

``` sh
make rel
```

### Run tests

``` sh
make test
```

Note: tests require `bats` installed and on `$PATH`.

### Manual smoke test (load a builtin)

``` sh
enable -f build/<tool>.debug.so <tool> || exit 99
printf "a b c\n" | <tool> ...
```

------------------------------------------------------------------------

# Agent execution protocol

When implementing a change, follow this sequence strictly.

Step 1 --- Identify the governing specification

-   locate the relevant file in `docs/`
-   identify the exact section describing the behavior

Step 2 --- Inspect the implementation

-   locate the builtin implementation
-   identify the functions responsible for the behavior

Step 3 --- Produce a minimal patch plan

-   list files to modify
-   list functions to modify
-   explain why the change is required

Step 4 --- Implement the change

-   modify only the required code
-   avoid refactoring unrelated logic

Step 5 --- Update tests

-   add or update Bats tests
-   cover happy path, edge case, and failure case

Step 6 --- Verify behavior

-   run `make test`
-   confirm exit codes match the contract
-   confirm streaming constraints remain intact

Builtin safety rule:

Any modification to a builtin must preserve the stability of the Bash
host process.

Verify that changes do not:

-   leak file descriptors
-   leave signal handlers altered
-   corrupt shell state
-   leave partially written stdout buffers

------------------------------------------------------------------------

# Non-negotiable suite invariants (follow exactly)

These invariants apply to **every builtin** unless its spec explicitly
states otherwise.

------------------------------------------------------------------------

## Streaming and complexity

Input must be processed **as a stream**.

Do not:

-   read entire stdin into memory
-   accumulate the full input file
-   build large in-memory arrays of records

Prefer:

-   incremental processing
-   fixed-size buffers
-   bounded memory usage
-   simple state machines

Memory usage should be **O(1)** or **O(k)** where `k` is small and
independent of total input.

Record order must be preserved unless a spec explicitly requires
reordering.

Streaming correctness rule:

A builtin must not change behavior depending on input size. Behavior
must remain correct for arbitrarily long streams.

------------------------------------------------------------------------

## Raw byte model and newline preservation

Input must be treated as **opaque bytes**.

Do not:

-   assume UTF-8
-   normalize whitespace
-   reinterpret byte sequences
-   trim newline characters unless the spec requires it

Preserve:

-   all bytes exactly
-   whitespace
-   whether the final line was unterminated

Never invent a trailing newline.

------------------------------------------------------------------------

## Exit codes (strict)

Exit code semantics are fixed across the entire suite:

    0 = success and output emitted
    1 = success and no output emitted
    2 = error (usage error or runtime error)

Implementation rule:

-   Do **not** return `0` when no output was emitted.
-   Track emission state explicitly if necessary.

------------------------------------------------------------------------

## CLI parsing discipline

All builtins must follow strict CLI parsing rules.

Required behavior:

-   `--help` prints usage and exits `0`
-   Unknown option before `--` → usage error → exit `2`
-   `--` terminates option parsing
-   Dash-leading tokens after `--` are treated as operands

Preferred implementation:

-   `getopt`-style parsing or equivalent logic
-   positional-primary invocation where project policy requires

Do not invent inconsistent parsing behavior.

------------------------------------------------------------------------

## SIGPIPE / stdout failure behavior

`SIGPIPE` should be treated as non-fatal, but **stdout write failure
must be detected**.

Implementation expectations:

At program termination:

1.  `fflush(stdout)`
2.  check `ferror(stdout)`

If either fails → **exit `2`**.

Do not rely solely on `fwrite()` return values.

------------------------------------------------------------------------

## Bash process safety

Loadable builtins must **never corrupt the hosting Bash process**.

Do not:

-   leak file descriptors
-   leave signals modified
-   corrupt shell state
-   allocate persistent memory that cannot be freed

A builtin must behave like a well-behaved library inside Bash.

------------------------------------------------------------------------

# Agent operating rules

Before modifying code:

1.  Identify the relevant specification sections
2.  Identify the specific files involved
3.  Propose a minimal patch plan
4.  Confirm expected exit code behavior
5.  Confirm streaming constraints

Do **not** edit files until the plan is clear.

Agents must **prefer minimal targeted patches** rather than large
rewrites.

------------------------------------------------------------------------

# Patch planning template

When proposing a change, structure the proposal like this:

    Patch plan:

    Spec references:
    - docs/<file>.md: <section>

    Files to modify:
    - src/...

    Functions affected:
    - function_name()

    Reason for change:
    - contract mismatch / missing behavior / bug

    Expected behavior after change:
    - describe outcome

    Risk analysis:
    - streaming impact
    - parsing impact
    - exit-code semantics

------------------------------------------------------------------------

# Test generation template

When adding tests, follow this structure:

    Test name:
    <short description>

    Spec reference:
    docs/<tool>.md

    Test cases:

    1. Happy path
       - expected stdout
       - expected exit code

    2. Edge case
       - unusual input condition

    3. Failure case
       - invalid usage → exit 2

    Assertions:
    - byte-exact stdout comparison
    - exit code verification

Tests must be deterministic and byte-exact when relevant.

------------------------------------------------------------------------

# Change discipline (how to work in this repo)

When implementing or modifying behavior:

1.  **Restate the relevant contract** (cite file path + section heading)
2.  Make the **smallest possible change**
3.  Update or add **Bats tests**
4.  Run:

```{=html}
<!-- -->
```
    make test

If formatting was touched:

    make format-check

or

    make format

if instructed.

Avoid:

-   refactoring unrelated code
-   renaming functions unnecessarily
-   changing CLI behavior unless the spec explicitly requires it

------------------------------------------------------------------------

# Review checklist (before proposing a patch)

-   [ ] Maintains streaming model
-   [ ] No unintended buffering introduced
-   [ ] Preserves raw byte behavior
-   [ ] Preserves newline semantics
-   [ ] Correct option parsing + `--` behavior
-   [ ] Correct exit code semantics (0/1/2)
-   [ ] Detects stdout write failure
-   [ ] Tests updated
-   [ ] Byte-exact expectations verified
-   [ ] Documentation updated if behavior changed

------------------------------------------------------------------------

# Communication style for proposals

Be concise and precise.

Always include:

-   spec reference
-   file path
-   function name
-   explanation of change
-   test updates
-   potential risks (streaming, parsing, buffering)

Avoid long explanations. Prefer concrete patch proposals.
