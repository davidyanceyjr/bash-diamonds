# `fields` — Diamond Builtin Specification (Week 2)

---

## 1. Purpose (One Sentence)

`fields` selects and emits specific 1-based fields from each input line, using numeric index/range specification, preserving line order and newline semantics deterministically.

---

## 2. Command Syntax

### 2.1 Synopsis

```
fields SPEC [--] [FILE...]
fields --help
```

### 2.2 Arguments

- `SPEC` (required): field selection specification (see Range Grammar).
- `FILE...` (optional): zero or more file paths. `-` means stdin.
- `--`: ends option parsing; subsequent tokens are treated as file names.

### 2.3 Options

- `--help`: print usage to stdout; exit 0.

### 2.4 Parsing Rules

- Only `--help` is recognized as an option.
- Any other `-x` token:
  - is a filename only if after `--`, or exactly `-`
  - otherwise usage error (exit 2).
- `SPEC` is the first non-option token unless `--help` is present.

---

## 3. Behavioral Specification

### 3.1 Input Sources

- If no `FILE` provided → read stdin.
- If files provided → process in order as concatenated stream.
- `-` denotes stdin at that position.
- Files read bytewise.
- No locale or encoding interpretation.

---

### 3.2 Line Definition

A line is:

- A maximal byte sequence ending in `\n`, or
- Final trailing bytes at EOF if no trailing newline.

Lines are processed sequentially and independently.

---

### 3.3 Field Definition

A field is defined as:

- A maximal sequence of non-whitespace bytes.
- Fields are separated by one or more ASCII whitespace characters.

Whitespace characters are:

```
' '   (space)
'\t'  (horizontal tab)
'\n'  (line feed — line terminator only, not part of fields)
'\r'
'\v'
'\f'
```

Important:

- Runs of whitespace collapse.
- Leading whitespace is ignored.
- Trailing whitespace is ignored.
- Empty fields are **not** produced.

This mirrors POSIX shell default whitespace splitting behavior.

No quoting, no escaping, no CSV logic.

---

### 3.4 Field Numbering

- Field numbering is **1-based per line**.
- Each line is independent.
- There is no global field numbering across lines.

Example:

```
a b c
d e
```

Line 1 fields: 1=a, 2=b, 3=c  
Line 2 fields: 1=d, 2=e

---

### 3.5 Output Semantics

For each input line:

1. Parse fields.
2. Apply normalized selection (ascending order, deduplicated).
3. Emit selected fields separated by a **single ASCII space**.
4. Preserve original line termination:
   - If original line ended with `\n`, output ends with `\n`.
   - If final line had no newline, output does not add one.

If no fields on a line match:

- Emit nothing for that line.
- Do not emit a blank line.

---

### 3.6 Determinism

Given identical input bytes and identical SPEC:

- Output bytes are identical.
- No locale dependence.
- No environment dependence.
- No filesystem ordering influence.
- No buffering-dependent behavior.

---

### 3.7 Memory and Streaming Constraints

`fields` must operate streaming:

- May buffer current line only.
- May buffer parsed/normalized selection structure.
- Must not buffer full input.
- May reuse line buffer for splitting.

---

### 3.8 Exit Codes (Diamond Rules)

- **0 (success with output)**:
  - At least one field was emitted across all lines.
- **1 (valid no result)**:
  - SPEC valid, input readable, but no fields selected anywhere.
- **2 (usage/runtime error)**:
  - Invalid SPEC
  - Missing SPEC
  - File open/read error
  - Write error
  - Invalid option
  - `--help` exits 0

---

## 4. Range Grammar

Identical to `lines` and implemented via shared selection logic.

### 4.1 Valid Examples

```
1
1,3
2..5
..3
4..
1, 3..5
```

### 4.2 Invalid Examples (exit 2)

```
0
01
-1
..
1,,
1,
,1
1..2..3
5..2
```

### 4.3 Normalization

- Deduplicate
- Sort ascending
- Reject reversed closed ranges
- Preserve open-ended ranges

---

## 5. Edge Case Matrix

| Case | Example | Required Behavior |
|-------|----------|------------------|
| Line with single field | `a` + `fields 1` | Emit `a` |
| Multiple fields | `a b c` + `fields 2` | Emit `b` |
| Open-ended range | `a b c` + `fields 2..` | Emit `b c` |
| Beyond field count | `a b` + `fields 5` | No output for that line |
| No matching fields in entire input | all lines shorter than selection | exit 1 |
| Leading whitespace | `   a b` + `fields 1` | Emit `a` |
| Multiple whitespace runs | `a   b` | Treated as single delimiter |
| Empty line | empty input line | Produces no output line |
| Line with only whitespace | `"   "` | Produces no output line |
| Final line no newline | `a b` (no `\n`) | Preserve lack of newline |
| Huge index overflow | > UINT64_MAX | exit 2 |

---

## 6. Internal API Reuse

### 6.1 Range Selection

Must reuse:

```
dc_sel_parse_and_normalize()
dc_sel_wants()
dc_sel_max_finite()
```

No duplicate parsing logic.

---

### 6.2 Field Splitting

`split.c` must provide:

```
size_t dc_split_ws(
    const uint8_t *line,
    size_t len,
    dc_field_view_t **out_fields
);
```

Where:

```
typedef struct {
    const uint8_t *ptr;
    size_t len;
} dc_field_view_t;
```

Constraints:

- Views reference line buffer.
- No per-field allocations of content.
- No copying of field content.
- May allocate temporary array for views.

---

## 7. Test Plan (BATS-Oriented)

Minimum matrix:

### Basic

- Single field
- Multiple field selection
- Closed range
- Open-ended
- Open-start

### Dedup/Overlap

- `2,2`
- `2..3,3`

### EOF / No Result

- Beyond max field index
- No fields match anywhere → exit 1

### Whitespace

- Leading whitespace
- Multiple spaces
- Tabs
- Mixed whitespace

### CLI

- `--help`
- Missing SPEC
- Invalid SPEC
- `--` handling
- File open error

### Newline Semantics

- Final line no newline
- Empty line handling

---

## 8. Documentation Requirements

`docs/fields.md` must include:

- Synopsis
- Field definition rules
- Range grammar summary
- Exit codes
- Input sources
- Whitespace semantics
- Newline preservation
- Non-goals

---

## 9. Explicit Non-Goals

`fields` will NOT:

- Support custom delimiter flags
- Support regex splitting
- Support CSV quoting
- Preserve original spacing
- Support negative indexing
- Support “from end” addressing
- Modify environment
- Perform sorting or reordering beyond selection normalization
- Support locale-aware classification
- Implement GNU `cut` feature parity

---

## 10. Design Philosophy

`fields` is not `cut`.

It is:

- deterministic
- minimal
- streaming
- composable with `lines`

Example pipeline:

```
ps aux | lines 2.. | fields 1,2,11..
```

This is the Diamond model.
