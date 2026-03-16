# Common Exit Codes

This page is non-normative guidance derived from `docs/project-spec.md`.
The authoritative suite contract remains `docs/project-spec.md`, especially:

- `2.7 Exit Codes`
- `2.8 Stdout Failure Handling`
- `2.9 Diagnostics`

## Suite-Wide Exit Codes

- `0`: success and output emitted
- `1`: success and no output emitted
- `2`: usage error, runtime error, I/O error, or stdout write failure

No other exit codes are permitted by the suite contract.

## Diagnostics and Stdout Failures

- Diagnostics go to stderr only.
- Normal output goes to stdout only.
- Builtins must not return `0` or `1` if final stdout flushing or error checks fail.
- Builtins must detect terminal stdout failure and return `2`.

## Builtin-Specific Notes

Some builtins document additional exit-`2` cases locally.

Some builtins also document narrower success behavior locally. For example,
`count` never exits with code `1` because it always emits a count on success.
