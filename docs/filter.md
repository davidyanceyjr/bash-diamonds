# filter — Diamond Builtin Specification (Week X)

Select input lines whose fields satisfy a constrained boolean expression.

This is NOT awk.
This is NOT a scripting language.
This is a minimal, deterministic WHERE-clause for delimited text.

-----------------------------------------------------------------------

## Synopsis

    filter EXPR [--] [FILE...]
    filter --help

-----------------------------------------------------------------------

## Exit Codes

  Code   Meaning
  ------ ---------------------------------------------------------------
  0      At least one matching line emitted
  1      No matching lines emitted (inputs readable)
  2      Usage error, expression parse error, file I/O error,
         stdout write error, or execution limit exceeded

SIGPIPE must be ignored internally so stdout write failures return exit 2.

-----------------------------------------------------------------------

## Input Semantics

- FILEs processed in order.
- `-` denotes stdin at that position.
- If no FILEs provided, read stdin.
- Streaming only: process one line at a time.
- No full-input buffering.
- Use shared dc_lr_* line reader.

Each line is treated as a record consisting of fields separated by
a single delimiter byte.

-----------------------------------------------------------------------

## Field Model

### Delimiter

- Default delimiter: TAB (`\t`, byte 0x09).
- No automatic whitespace trimming.
- No CSV semantics.
- No quoting rules.
- Delimiter is literal and byte-based.
- Empty fields between consecutive delimiters are allowed.

Example (TAB-separated):

    user<TAB>age<TAB>days

### Field Indexing

- Fields are 1-based.
- `$1` refers to first field.
- `$2` refers to second field.
- Referencing a field beyond the number of fields in the line yields
  an empty string.

-----------------------------------------------------------------------

## Output Semantics

- If EXPR evaluates true for a line:
  - emit the original line bytes exactly as read,
    including newline if present.
- If false:
  - emit nothing for that line.
- Unterminated final line:
  - if true, emit unterminated.

Newline is structural and not part of any field.

-----------------------------------------------------------------------

## Expression Language (v1)

The expression language is intentionally minimal.

### Operands

1. Field references:
       $1, $2, $3, ...
   - Must be positive integers.
   - `$0` is invalid (parse error).

2. Integer literals:
       0
       -10
       42
   - Base-10 only.
   - No hex, no octal.
   - No floating point.

3. String literals:
       "text"
   - Supports only:
       \"   (double quote)
       \\   (backslash)
   - No other escape sequences allowed.
   - Unterminated string is parse error.

Barewords are NOT supported.

-----------------------------------------------------------------------

## Operators

### Comparison Operators

    ==  !=  <  <=  >  >=

Evaluation rules:

- If both operands parse as valid base-10 signed integers:
    comparison is numeric.
- Otherwise:
    comparison is bytewise string comparison.
    (unsigned byte order, no locale influence)

String comparison is exact and case-sensitive.

### Boolean Operators

    &&   ||   !

- `!` is unary prefix.
- `&&` has higher precedence than `||`.
- Parentheses `(` `)` allowed for grouping.

Operator precedence (highest to lowest):

    1. !
    2. comparison operators
    3. &&
    4. ||

-----------------------------------------------------------------------

## Grammar (informal)

- expr        := or_expr
- or_expr     := and_expr ( '||' and_expr )*
- and_expr    := unary_expr ( '&&' unary_expr )*
- unary_expr  := '!' unary_expr
               | primary
- primary     := comparison
               | '(' expr ')'
- comparison  := operand comp_op operand
- operand     := field_ref
               | int_literal
               | string_literal

Restrictions:

- Empty expression is parse error.
- Missing operand is parse error.
- Invalid operator placement is parse error.
- `$` not followed by digits is parse error.
- `$0` is parse error.

-----------------------------------------------------------------------

## Determinism and Limits

To ensure predictable runtime:

- Maximum EXPR length: 4096 bytes
- Maximum tokens: 2048
- Maximum AST nodes: 2048
- Maximum per-line evaluation step budget: 500,000 operations

The step budget:

- Applies per input line.
- Resets for each new line.
- Counts each AST node evaluation and logical operation.

If evaluation exceeds the step budget:

    filter: expression evaluation limit exceeded

Exit 2.

-----------------------------------------------------------------------

## Error Handling

Exit 2 for:

- Missing EXPR
- Unknown option
- Expression parse error
- File open/read error
- Stdout write error
- Evaluation limit exceeded

All errors print exactly one line to stderr:

    filter: <message>

-----------------------------------------------------------------------

## Non-Goals

- No regex matching (use `match`)
- No substring operators
- No arithmetic (`+ - * /`)
- No variables
- No assignments
- No functions
- No floating point
- No field mutation
- No output formatting
- No CSV parsing
- No locale-aware behavior

-----------------------------------------------------------------------

## Examples

Select users older than 25:

    filter '$2 > 25' users.tsv

Select records where age > 25 and days > 83:

    filter '$2 > 25 && $3 > 83' users.tsv

Select records where status is exactly "ERROR":

    filter '$1 == "ERROR"' logs.tsv

Combine with match:

    match 'smith' users.tsv | filter '$2 > 25 && $3 > 83'