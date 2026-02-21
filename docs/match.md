# match — Diamond Builtin Specification (Week 3)

Filter input lines by a deterministic, constrained regex.

This is NOT GNU grep.
This is a minimal, pipeline-first line filter.

-----------------------------------------------------------------------

## Synopsis

    match PATTERN [--] [FILE...]
    match --help

-----------------------------------------------------------------------

## Exit Codes

  Code   Meaning
  ------ ---------------------------------------------------------------
  0      At least one matching line emitted
  1      No matching lines emitted (inputs readable)
  2      Usage error, pattern compile error, file I/O error,
         stdout write error, or execution limit exceeded

SIGPIPE must be ignored internally so stdout write failures return exit 2.

-----------------------------------------------------------------------

## Input Semantics

Same concatenation rules as `lines`, `fields`, and `trim`:

- FILEs processed in order.
- `-` denotes stdin at that position.
- If no FILEs provided, read stdin.
- Use shared `dc_lr_*` line reader (streaming, line-at-a-time).
- Only the current line may be retained; no full-input buffering.

-----------------------------------------------------------------------

## Output Semantics

- For each input line, if it matches, emit the original line bytes
  exactly as read, including its newline if present.
- If it does not match, emit nothing for that line.
- Unterminated final line:
  - if it matches, emit it unterminated (do not append '\n').

Newline is structural and is not part of the match subject.

-----------------------------------------------------------------------

## Match Subject

Let the input line be bytes B[0..len-1].

- If the line ends with '\n', the match subject is B[0..len-2].
- Otherwise, the match subject is B[0..len-1].

The subject is treated as a byte array with explicit length.

Pattern bytes are taken from argv and therefore cannot contain NUL.

The regex operates on bytes (0x00–0xFF).
This v1 language defines portable behavior only for ASCII-range constructs.

-----------------------------------------------------------------------

## Option Parsing Rules

- `--help` is recognized only when it is the sole argument.
- Any other `-x` token before `--` is a usage error unless the token is
  exactly `-`.
- `--` ends option parsing.
- `--` may appear only after PATTERN.
- If argv[1] is `--`, this is a usage error (missing PATTERN).
- After `--`, dash-leading filenames are allowed.
- Token `-` in FILE list denotes stdin at that position.

To match a pattern beginning with `-`, use:

    match -- '-foo' file

-----------------------------------------------------------------------

## PATTERN Requirements

- PATTERN must be 1–4096 bytes.
- Empty PATTERN is a compile error (exit 2).

-----------------------------------------------------------------------

## Regex Language (v1)

The language is a constrained ERE-like subset.

### Metacharacters

Metacharacters are:

    . * + ? | ( ) [ ] ^ $ \

A metacharacter is literal only when escaped with `\`,
except inside `[...]` where class rules apply.

-----------------------------------------------------------------------

## Anchors

- `^` is an anchor only if it is the first byte of PATTERN.
- `$` is an anchor only if it is the last byte of PATTERN.
- Anchors inside groups are treated as literal bytes.
- Anchors elsewhere are literal unless escaped.

Examples:

- `^foo`        → anchored at subject start
- `(foo)$`      → anchored at subject end
- `(^foo)`      → `^` is literal
- `foo$|bar`    → `$` is literal

-----------------------------------------------------------------------

## Grammar (informal)

- regex        := alt
- alt          := concat ( '|' concat )*
- concat       := repeat+
- repeat       := atom quant?
- quant        := '*' | '+' | '?'
- atom         := literal
               | '.'
               | group
               | class
               | anchor_start
               | anchor_end

Restrictions:

- Empty alternates are compile errors (`a|`, `|a`).
- Empty groups `()` are compile errors.
- Empty classes `[]` or `[^]` are compile errors.
- Pattern may not begin with a quantifier.
- Quantifier may not follow `|` or `(`.
- Double quantifiers (`a**`, `a+?`, etc.) are compile errors.
- Unterminated group is compile error.
- Unterminated class is compile error.
- Trailing escape (`\` at end of pattern) is compile error.

-----------------------------------------------------------------------

## Literals and Escapes

Valid escapes:

    \. \* \+ \? \| \( \) \[ \] \^ \$ \\

Any other `\X` is a compile error.

-----------------------------------------------------------------------

## Dot

`.` matches any single byte in the subject.

-----------------------------------------------------------------------

## Character Classes

`[...]` matches exactly one byte.

Supported forms:

- Explicit bytes: `[abc]`
- Ranges: `[a-z]`, `[0-9]`
- Negation: `[^a-z]`

Rules:

- `^` is negation only if it is the first byte after `[`.
- `-` is a range operator only when between two bytes; otherwise literal.
  - `[a-]` and `[-a]` are valid and include `-`.
- Ranges must be strictly increasing in unsigned byte value.
  - `z-a` is compile error.
- `]` must be escaped (`\]`) to be literal.
- Supported escapes inside classes:
  - `\\`
  - `\]`
  - `\-`
  - `\^`
- POSIX named classes (e.g. `[[:alpha:]]`) are compile errors.

-----------------------------------------------------------------------

## Matching Mode

Default mode is search.

For each subject:

- The engine attempts a match starting at subject offsets 0 through len.
- First successful match stops evaluation of that line.
- If PATTERN begins with `^`, matching is attempted only at offset 0.
- `$` requires match to end at subject length.

Quantifiers are greedy, but only match existence is observable.
Match spans are not exposed.

-----------------------------------------------------------------------

## Execution Model and Resource Limits

Implementation uses Thompson NFA simulation with explicit epsilon-closure.

- Active state sets must not contain duplicate instructions.
- State processing order must be deterministic.

Limits:

- Maximum PATTERN length: 4096 bytes
- Maximum compiled NFA instruction count: 16384
- Maximum active NFA states: 8192
- Maximum per-line transition budget: 2,000,000

The transition budget:

- Applies per input line.
- Resets for each new line.
- Includes all restart attempts for that line.
- Counts each time an NFA instruction/state is processed for a subject
  position, including epsilon transitions.

If the active state set exceeds 8192 entries,
or if the transition budget is exceeded:

    match: regex execution limit exceeded

Exit 2.

If compilation exceeds the instruction limit,
pattern compilation fails (exit 2).

These limits guarantee predictable runtime and prevent pathological
behavior.

-----------------------------------------------------------------------

## Error Handling

Exit 2 for:

- Missing PATTERN
- Unknown option or invalid option placement
- Pattern compile error
- File open/read error
- Stdout write error
- Regex execution limit exceeded

All errors print exactly one line to stderr:

    match: <message>

-----------------------------------------------------------------------

## Non-Goals

- No flags (`-i`, `-v`, `-n`, etc.)
- No multi-line matching
- No capture groups or backreferences
- No lookahead/lookbehind
- No POSIX named classes
- No locale or UTF-8 semantics

-----------------------------------------------------------------------

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