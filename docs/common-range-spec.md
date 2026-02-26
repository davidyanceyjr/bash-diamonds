# Range selection SPEC (shared)

Used by `lines` and `fields`.

## Grammar (1-based)

- `N` single index
- `N,M` list
- `A..B` closed range
- `..B` open start
- `A..` open end

Whitespace is allowed around `,` and `..`.

## Normalization

The implementation parses and normalizes the selection:

- Sorted ascending
- Duplicates removed
- Overlapping ranges merged

Errors (exit 2):

- Reversed ranges (`3..1`)
- Leading zeros (`01`)
- Bare `..`
- Trailing/double commas
- uint64 overflow