# Bash Diamonds

A practical user guide for readable Bash text pipelines.

This handbook is non-normative. For authoritative behavior and guarantees, see `docs/project-spec.md`.

## What Bash Diamonds Is

Bash Diamonds is a suite of Bash loadable builtins for line-oriented text processing.

Design goals:
- small composable tools
- predictable exit semantics
- byte-exact newline behavior

## Load a Builtin

```bash
enable -f build/fields.debug.so fields
```

Typical development build:

```bash
make debug
```

## User-Level Install

From a repository clone (after `make rel`):

```bash
./scripts/install-user.sh --source ./build
```

From a release tarball root:

```bash
./install-user.sh
```

The script installs `.so` files into `~/.local/lib/bash-diamonds/` and writes a
loader snippet to `~/.bashrc` so builtins are available in new shells.

Use `./install-user.sh --help` for options (`--source`, `--prefix`, `--no-rc`).

## Core Tools (Current)

Stable:
- `lines SPEC [--] [FILE...]`
- `fields SPEC [FILE...]`
- `match PATTERN [--] [FILE...]`
- `take N [S] [--] [FILE...]`
- `trim [--] [FILE...]`
- `table [--] [FILE...]`
- `count [--] [FILE...]`
- `filter EXPR [--] [FILE...]`
- `replace [--literal] PATTERN REPLACEMENT [--] [FILE...]`

## Exit Codes (All Tools)

- `0`: success with output emitted
- `1`: success with no output emitted
- `2`: usage/runtime/write error

## Practical Examples

Extract, match, count:

```bash
printf "a 1\nb 2\na 3\n" | fields 1 | match '^a$' | count
```

Select line ranges:

```bash
printf "x\ny\nz\n" | lines 2..
```

Take first N lines:

```bash
printf "a\nb\nc\n" | take 2
```

Trim leading/trailing ASCII whitespace:

```bash
printf "  apple  \n\tpear\t\n" | trim
```

Literal replacement, emit only changed lines:

```bash
printf "foo\nbar\n" | replace --literal foo baz
```

## CLI Notes

- Use `--help` for builtin-specific help.
- `--` ends option parsing.
- A dash-leading operand usually requires `--` before it.

## Streaming Notes

Most builtins are streaming.

`table` is intentionally two-pass and requires seekable regular files (stdin/`-` is rejected).

## Scope Boundaries

Use Bash Diamonds when you want clear, deterministic shell pipelines.

Use other tools when you need full programming-language parsing or heavy transformations.
