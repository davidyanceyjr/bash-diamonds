# `match` — Diamond Builtin Specification (Week 3)

Filter input lines by a deterministic, constrained regex.

This is NOT GNU `grep`.
This is a minimal, pipeline-first line filter.

------------------------------------------------------------------------

## Synopsis

    match PATTERN [--] [FILE...]
    match --help

------------------------------------------------------------------------

## Exit Codes

  Code   Meaning
  ------ ---------------------------------------------------------------
  0      At least one matching line emitted
  1      No matching lines emitted (inputs readable)
  2      Usage error, pattern compile error, file I/O error, stdout write error

SIGPIPE must be ignored internally so stdout write failures return exit 2.

------------------------------------------------------------------------

## Input Semantics

Same concatenation rules as `lines` / `fields` / `trim`:

- FILEs processed in order.
- `-` denotes stdin at that position.
- If no FILEs provided, read stdin.
- Use shared dc_lr_* line reader (streaming, line-at-a-time).
- Only the current line may be retained; no full-input buffering.

------------------------------------------------------------------------

## Output Semantics

- For each input line, if it matches, emit the original line bytes exactly as read:
  - including its newline if present.
- If it does not match, emit nothing for that line.
- Unterminated final line:
  - if it matches, emit it unterminated (do not append '\n').

Newline is *structural*: it is not part of the match subject.

------------------------------------------------------------------------

## Match Subject

Let the input line be bytes `B[0..len-1]`.

- If the line ends with `\n`, the match subject is `B[0..len-2]`.
- Otherwise (unterminated), the match subject is `B[0..len-1]`.

The regex operates on bytes (0x00–0xFF), but this v1 language only defines
portable behavior for ASCII-range constructs (literals, ranges).
No locale/UTF-8 semantics.

------------------------------------------------------------------------

## Option Parsing Rules

- Only `--help` is recognized.
- Any other `-x` token before `--` is a usage error unless token is exactly `-`.
- `--` ends option parsing.
- After `--`, dash-leading filenames are allowed.
- Token `-` in FILE list denotes stdin at that position.

------------------------------------------------------------------------

## Regex Language (v1)

The language is a constrained ERE-like subset.

### Metacharacters

Metacharacters are: `. * + ? | ( ) [ ] ^ $ \`

A metacharacter is literal only when escaped with `\`, except:
- Inside `[...]`, `]`, `-`, `^`, and `\` have special rules (below).

### Grammar (informal)

- regex        := alt
- alt          := concat ( '|' concat )*
- concat       := repeat*
- repeat       := atom quant?
- quant        := '*' | '+' | '?'
- atom         := literal
               | '.'
               | group
               | class
               | anchor_start
               | anchor_end

- group        := '(' alt ')'
- class        := '[' class_body ']'
- anchor_start := '^' (only if it is the first token of the pattern)
- anchor_end   := '$' (only if it is the last token of the pattern)

Notes:
- Empty alternates are NOT allowed: `a|` and `|a` are compile errors.
- Empty groups are NOT allowed: `()` is a compile error.
- Empty classes are NOT allowed: `[]` or `[^]` are compile errors.

### Literals

Any byte other than metacharacters matches itself.
Escapes produce literal bytes for metacharacters:

  \. \* \+ \? \| \( \) \[ \] \^ \$ \\

Any other escape sequence `\X` where X is not one of the above is a
pattern compile error (exit 2).

### Dot

`.` matches any single byte in the subject.

### Character Classes

`[...]` matches exactly one byte.

Supported forms:
- Explicit bytes: `[abc]`
- Ranges: `[a-z]`, `[0-9]` (ASCII codepoint order only)
- Negation: `[^a-z]`

Escapes inside classes:
- `\\` matches backslash
- `\]` matches `]`
- `\-` matches `-`
- `\^` matches `^` (when used literally)

Rules:
- `^` is negation only if it is the first character after `[` (or after `[^`).
- `-` is a range operator only when between two bytes; otherwise literal.
- Ranges must be increasing by byte value (e.g. `z-a` is compile error).

POSIX named classes (e.g. `[[:alpha:]]`) are NOT supported (compile error).

### Quantifiers

`*`, `+`, `?` apply to the immediately preceding atom or group.

Invalid uses are compile errors:
- pattern begins with a quantifier
- quantifier follows `|` or `(` or begins a concat
- double quantifiers like `a**`, `a+?` etc.

### Alternation and Grouping

- `|` alternation has lowest precedence.
- Concatenation binds tighter than `|`.
- Parentheses group as expected.
- Nested groups allowed.

------------------------------------------------------------------------

## Matching Mode

Default mode is **search**: a match may occur anywhere in the subject.
Anchors restrict as usual:
- `^` at pattern start anchors at subject position 0.
- `$` at pattern end anchors at subject end.

Anchors elsewhere are literals unless escaped rules apply.

------------------------------------------------------------------------

## Determinism and Resource Limits (v1)

To ensure predictable behavior:

- Maximum PATTERN length: 4096 bytes
- Maximum compiled program size (NFA instructions): 16384
- Maximum active NFA states during execution: 8192
- Maximum per-line “step” budget: 2,000,000 state transitions
  - If exceeded, treat as runtime error (exit 2) with message:
    "match: regex execution limit exceeded"

Rationale: prevents pathological regexes from hanging the shell.

------------------------------------------------------------------------

## Error Handling

Exit 2 for:
- Missing PATTERN
- Unknown option / invalid option placement
- Pattern compile error (invalid syntax, unsupported constructs)
- File open/read error
- Stdout write error (including closed stdout)
- Regex execution limit exceeded

------------------------------------------------------------------------

## Examples

Match any line containing "error":
    match 'error' file.log

Match lines starting with YYYY-MM-DD:
    match '^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]' file.log

Match lines ending with ".c":
    match '\.c$' < files.txt

Match "foo" or "bar":
    match '(foo|bar)' input

Match hex bytes:
    match '0x[0-9A-Fa-f]+' input
