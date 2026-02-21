# Week 3 Plan — Presentation + Filtering Primitives

Week 1 delivered: `lines`, `fields` (green tests on main).
Week 2 delivered: `trim` (green tests on feature/trim).

Week 3 goal:
Add two small, composable primitives that improve interactive workflows:
- `table` (formatting)
- `match` (constrained filtering)

Both MUST follow Diamond Rules:
- Minimal surface area, deterministic behavior, pipeline-first
- Streaming only (line-at-a-time)
- Exit codes: 0=emitted, 1=no result, 2=usage/runtime error
- No environment mutation
- Deterministic newline semantics
- SIGPIPE ignored internally (stdout failures return 2)

Acceptance:
- Each builtin has a complete spec in docs/
- Each builtin has a Bats test file covering behavior + exit codes
- `make test` is green
