---

# replace

## Name

replace — perform per-line substitution and emit only modified lines

---

## Synopsis

```
replace [--literal] PATTERN REPLACEMENT [--] [FILE...]
replace --help
```

Primary form is positional: `replace [--literal] PATTERN REPLACEMENT [--] [FILE...]`.

---

## Description

`replace` performs substitution on each input record (line).

For every input record:

* Matching and replacement operate on the **record content excluding the terminating newline byte (`0x0A`)**, if present.
* If at least one substitution occurs in that record, the modified record is written to standard output.
* If no substitution occurs, the record is not written.

`replace` never adds, removes, or modifies record delimiters.
If an input record was terminated by a newline byte, the emitted record will be terminated by a newline byte.
If the final input record is unterminated, the emitted record remains unterminated.

---

## Modes

### Default (regex mode)

`PATTERN` is interpreted as a POSIX Extended Regular Expression (ERE).

* All non-overlapping matches within a record are replaced.
* Matching is performed left-to-right.
* After a replacement, scanning resumes at the end of the replaced span.
* Zero-length matches must advance at least one byte to prevent infinite loops.

### Literal mode

```
--literal
```

`PATTERN` is treated as an exact byte sequence.

* All non-overlapping occurrences are replaced.
* Matching proceeds left-to-right.
* Empty `PATTERN` is a usage error.

---

## Replacement

`REPLACEMENT` is treated as literal bytes.

* No escape sequences are processed.
* No backreferences are supported.
* Empty `REPLACEMENT` is allowed and deletes matches.

---

## Input

`FILE...` specifies input files processed in order.

If no files are provided:

* Standard input is read.

A file operand of `-` means standard input at that position.

File open or read errors are fatal (exit 2).

---

## Output

For each input record:

* If ≥1 substitution occurred: emit the modified record.
* If no substitution occurred: emit nothing.

Output preserves:

* All record content bytes except those replaced.
* The presence or absence of the terminating newline byte.

No newline is synthesized.

---

## Exit Status

| Code | Meaning                                                          |
| ---- | ---------------------------------------------------------------- |
| 0    | At least one record emitted (≥1 substitution occurred somewhere) |
| 1    | No substitutions occurred (no output)                            |
| 2    | Error (usage, invalid regex, I/O error, stdout write failure)    |

---

## Strict Option Parsing

`replace` follows the Bash Diamonds strict parsing rules:

* `--help` prints usage to stdout and exits 0.
* `--` ends option parsing.
* Unknown options before `--` are usage errors (exit 2).
* Use `--` before dash-leading file operands.
* `--literal` must appear before `PATTERN`.
* Duplicate `--literal` is a usage error.
* Empty `PATTERN` is a usage error.

If `PATTERN` begins with `-`, callers must use:

```
replace -- PATTERN REPLACEMENT ...
```

or:

```
replace --literal -- PATTERN REPLACEMENT ...
```

---

## Binary Safety

* Input is processed as raw bytes.
* Records are delimited by `0x0A` when present.
* Matching and replacement operate only on record content (excluding the delimiter).
* Bytes other than `0x0A` have no special meaning unless interpreted by the regex engine.

---

## Streaming Model

`replace` processes input record-by-record.

* Only the current record is buffered.
* The entire input is never buffered.
* Behavior is deterministic across multiple files.

---

## SIGPIPE Handling

The builtin ignores `SIGPIPE` internally.

If writing to stdout fails:

* `replace` exits with status 2.

---

## Examples

Replace all occurrences of `foo` with `bar`:

```
replace foo bar file.txt
```

Delete all digits from input:

```
replace '[0-9]' '' data.txt
```

Literal replacement of `--` with `:`:

```
replace --literal -- -- : file.txt
```

---
