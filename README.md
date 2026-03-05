# Bash Diamonds

Bash Diamonds is a suite of Bash loadable builtins for line-oriented text processing.

Normative contract:
- `docs/project-spec.md`

User handbook:
- `docs/diamond-suite.md`

## Prerequisites

- Bash with loadable builtin support
- C toolchain (`cc`, `make`)
- `bats` on `PATH` for tests
- Optional: `clang-format` for formatting checks

## Quickstart

Build debug shared objects:

```sh
make debug
```

Load a builtin:

```sh
enable -f build/fields.debug.so fields
```

Run a simple pipeline:

```sh
printf "a 1\nb 2\na 3\n" | fields 1 | match '^a$' | count
```

## Test

Run the conformance suite:

```sh
make test
```

## Release Bundle

Build a release bundle with builtins and docs:

```sh
make dist
```

Artifacts are written to `build/release/`:
- `diamonds-<version>.tar.gz`
- `diamonds-<version>.tar.gz.sha256`

Suite-wide exit codes:
- `0`: success and output emitted
- `1`: success and no output emitted
- `2`: usage/runtime/write error

## Repository Layout

- `src/builtins/`: builtin implementations (`builtin_<name>.c`)
- `src/diamondcore/`: shared core code
- `src/include/` and `src/builtins/`: shared headers
- `tests/`: Bats conformance tests
- `docs/`: specs and handbook

## Development Notes

- Treat `docs/project-spec.md` as source of truth for behavior.
- Keep patches small and contract-preserving.
- Builtins are expected to process input as streams unless specified otherwise.
