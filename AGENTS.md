# AGENTS.md — Bash Diamonds (Codex Agent Instructions)

This repository contains **Bash loadable builtins** (C) designed for **streaming pipelines**.

## Source of truth

Read these first and treat them as normative contracts:

- `docs/project-spec.md` (suite-wide invariants and exit-code model)
- `docs/development-governance.md` (workflow and doc roles)
- `docs/<tool>.md` (per-builtin spec; one file per tool)

If documentation and code disagree, **do not “fix” behavior implicitly**:
- report the mismatch clearly
- propose the smallest change that restores compliance to the spec
- update tests to lock the contract

## Repository map

- Builtins: `src/builtins/builtin_<name>.c`
- Shared core: `src/diamondcore/*.c`
- Shared headers: `src/include/**` and `src/builtins/**` (as applicable)
- Tests: `tests/*.bats`
- Docs/specs: `docs/`

## Build and test

### Build (debug)
```sh
make debug
```

### Build (release)
```sh
make rel
```

### Run tests
```sh
make test
```
Note: tests require `bats` to be installed and on `$PATH`.

### Manual smoke test (load a builtin)
```sh
enable -f build/<tool>.debug.so <tool> || exit 99
printf "a b c\n" | <tool> ...
```

## Non-negotiable suite invariants (follow exactly)

These invariants apply to **every** builtin unless its spec explicitly says otherwise.

### Streaming and complexity
- **Stream input**; avoid full buffering of stdin or full-file reads.
- Keep memory bounded; prefer O(1)/O(k) where k is small and independent of total input.
- Preserve record order; do not reorder unless the tool’s spec requires it.

### Raw byte model and newline preservation
- Treat input as raw bytes; do not assume UTF-8.
- Preserve bytes exactly (including whitespace) except where the tool’s spec transforms them.
- Preserve whether the final line was unterminated (do not “invent” a trailing newline).

### Exit codes (strict)
- `0` = success **and emitted output**
- `1` = success **and emitted no output** (true “no result”)
- `2` = **error** (usage errors and runtime errors)

Do not drift from this model.

### CLI parsing discipline
- Support `--help` and show usage; `--help` exits `0`.
- Unknown option before `--` is a **usage error** → exit `2`.
- `--` terminates option parsing; after `--`, treat dash-leading operands as operands.
- Prefer positional-primary invocation (per project policy), but preserve existing flag forms where required.

### SIGPIPE / stdout failure behavior
- Treat `SIGPIPE` as non-fatal (ignore or handle), but **detect output write failure**.
- If stdout write fails (including flush failure), exit `2`.

Implementation expectation:
- Do not rely solely on `fwrite()` return values.
- Ensure end-of-program flush/error checks are performed (`fflush(stdout)` and/or `ferror(stdout)`).

## Change discipline (how to work in this repo)

When asked to implement or modify something:

1. **Start by restating the relevant contract** (quote file paths + section headings, not long excerpts).
2. Make the **smallest possible change** that satisfies the contract.
3. Update or add **Bats tests** to cover:
   - happy path
   - at least one edge case
   - at least one failure path (exit `2`)
4. Run:
   - `make test`
   - if formatting was touched: `make format-check` (or `make format` if instructed)
5. Keep diffs tight:
   - don’t refactor unrelated code
   - don’t rename public CLI flags/behavior unless explicitly requested
   - don’t change stable tool semantics unless the spec says it’s wrong

## Review checklist (before proposing a patch)

- [ ] Maintains streaming model (no hidden buffering)
- [ ] Preserves raw bytes/newline behavior per spec
- [ ] Strict option parsing + correct `--` behavior
- [ ] Correct exit code semantics (0/1/2)
- [ ] Detects stdout write failure (flush + error)
- [ ] Tests updated and deterministic (byte-level where relevant)
- [ ] Docs updated if behavior/usage text changed

## Communication style for proposals

Be direct and specific:
- identify contract section(s)
- identify file(s) + function(s)
- explain why the change is required
- show exact test additions/updates
- call out any risks (especially buffering or parsing regressions)
