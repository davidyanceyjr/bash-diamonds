# `count`

## Synopsis

count [--] [FILE...]
count --help

Primary form is positional: `count [--] [FILE...]`.

## Description

`count` reads input as a stream of lines and prints the number of lines read as an unsigned decimal integer followed by `\n`.

A final unterminated line counts as a line.

## Options

* `--help` print help to stdout and exit 0
* `--` end option parsing
* unknown option before `--` is a usage error and exits `2`
* use `--` before dash-leading file operands

No other options are supported.

## Arguments

* `FILE...` input files

Behavior:

* If no `FILE` is provided, read from standard input.
* A `FILE` of `-` means standard input.
* `-` may appear among other files and must be processed in order.
* Filenames beginning with `-` must be preceded by `--`.

## Input semantics

* Input is processed as a sequence of lines.
* A line is:
  * A maximal byte sequence ending in `\n`, or
  * The final byte sequence at EOF even if it does not end in `\n`.
* The final unterminated line counts as a line.
* No bytes are interpreted or modified.

## Output semantics

* On success, print the line count as base-10 unsigned decimal followed by a single newline.
* Output is written to stdout only.

## Exit Codes

See `docs/common-exit-codes.md`.

`count` never exits with code `1`; on success it always emits a count, including `0`.

## SIGPIPE

* SIGPIPE must be ignored.
* Any stdout write failure → exit 2.
