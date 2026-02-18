#!/usr/bin/env bats

# tests/lines.bats
#
# Assumptions:
# - build/lines.debug.so exists (built via `make`)
# - bats-core is installed
#
# Run:
#   bats tests/lines.bats
# or:
#   make test   (if your Makefile wires it)

setup() {
  ROOT="${BATS_TEST_DIRNAME}/.."
  LINES_SO="${LINES_SO:-$ROOT/build/lines.debug.so}"

  if [[ ! -f "$LINES_SO" ]]; then
    echo "missing lines so: $LINES_SO" >&2
    return 2
  fi

  TMPDIR="${BATS_TEST_TMPDIR:-/tmp}"
  F1="$TMPDIR/lines_f1.txt"
  F2="$TMPDIR/lines_f2.txt"
}

# Helper: run lines with stdin, capture stdout bytes exactly via od.
# Usage: run_lines_od "SPEC" "INPUT"
run_lines_od() {
  local spec="$1"
  local input="$2"
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    printf '%s' \"$input\" | lines $spec | od -An -tx1
  "
}

# Helper: run lines, capture raw stdout (good for simple cases)
run_lines() {
  local spec="$1"
  local input="$2"
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    printf '%s' \"$input\" | lines $spec
  "
}

@test "lines: single index selects line 2 (exit 0)" {
  run_lines "2" $'a\nb\nc\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'b' ]
}

@test "lines: multiple indices 1,3 (exit 0)" {
  run_lines "1,3" $'a\nb\nc\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nc' ]
}

@test "lines: closed range 2..3 (exit 0)" {
  run_lines "2..3" $'a\nb\nc\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'b\nc' ]
}

@test "lines: open start ..2 (exit 0)" {
  run_lines "..2" $'a\nb\nc\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nb' ]
}

@test "lines: open end 2.. (exit 0)" {
  run_lines "2.." $'a\nb\nc\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'b\nc' ]
}

@test "lines: duplicate index dedupes (2,2 => one line) (exit 0)" {
  run_lines "2,2" $'a\nb\nc\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'b' ]
}

@test "lines: overlapping ranges merge/dedupe (2..3,3..4) (exit 0)" {
  run_lines "2..3,3..4" $'a\nb\nc\nd\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'b\nc\nd' ]
}

@test "lines: unsorted spec normalizes (3,1 => emits 1 then 3) (exit 0)" {
  run_lines "3,1" $'a\nb\nc\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nc' ]
}

@test "lines: select beyond EOF => no output, exit 1" {
  run_lines "10" $'a\nb\n'
  [ "$status" -eq 1 ]
  [ "$output" = "" ]
}

@test "lines: partially beyond EOF (2..10) emits existing, exit 0" {
  run_lines "2..10" $'a\nb\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'b' ]
}

@test "lines: empty input => exit 1" {
  run_lines "1" ""
  [ "$status" -eq 1 ]
  [ "$output" = "" ]
}

@test "lines: no trailing newline selected preserves bytes (line 2 is 'b' no \\n)" {
  run_lines_od "2" $'a\nb'
  [ "$status" -eq 0 ]
  # output is hex bytes from od, whitespace-variant; match the byte sequence "62"
  [[ "$output" =~ (^|[[:space:]])62($|[[:space:]]) ]]
  # ensure no 0a (newline) present
  [[ ! "$output" =~ (^|[[:space:]])0a($|[[:space:]]) ]]
}

@test "lines: no trailing newline not selected (line 1 includes \\n)" {
  run_lines_od "1" $'a\nb'
  [ "$status" -eq 0 ]
  # 'a' = 61, '\n' = 0a
  [[ "$output" =~ (^|[[:space:]])61($|[[:space:]]) ]]
  [[ "$output" =~ (^|[[:space:]])0a($|[[:space:]]) ]]
}

@test "lines: reversed range (3..1) => exit 2" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    printf 'a\nb\nc\n' | lines 3..1 >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "lines: leading zero (01) => exit 2" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    printf 'a\nb\n' | lines 01 >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "lines: bare '..' => exit 2" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    printf 'a\nb\n' | lines .. >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "lines: uint64 overflow => exit 2" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    printf 'a\nb\n' | lines 18446744073709551616 >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "lines: concatenation numbering across files (line 3 is first line of file2)" {
  printf 'a\nb\n' >"$F1"
  printf 'c\nd\n' >"$F2"

  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    lines 3 '$F1' '$F2'
  "
  [ "$status" -eq 0 ]
  [ "$output" = $'c' ]
}

@test "lines: stdin in middle (files: f1 - f2); select line 2" {
  printf 'a\nb\n' >"$F1"
  printf 'c\nd\n' >"$F2"

  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    printf 'X\n' | lines 2 '$F1' - '$F2'
  "
  [ "$status" -eq 0 ]
  [ "$output" = $'b' ]
}

# --------------------------
# Additional CLI/contract tests
# --------------------------

@test "lines: --help prints usage to stdout, exit 0" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    lines --help
  "
  [ "$status" -eq 0 ]
  [[ "$output" =~ usage: ]]
}

@test "lines: missing SPEC => exit 2" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    lines >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "lines: unknown -x token before -- is usage error (exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    printf 'a\n' | lines -x >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "lines: '-' in FILE list denotes stdin at that position" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    printf 'a\nb\n' | lines 1 -
  "
  [ "$status" -eq 0 ]
  [ "$output" = $'a' ]
}

@test "lines: '--' ends option parsing; -name after -- is treated as filename (missing => exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    lines 1 -- -no_such_file >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "lines: '--' allows dash-leading filenames (existing file succeeds)" {
  local dashfile="$TMPDIR/-dashfile"
  printf 'z\n' >"$dashfile"

  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    lines 1 -- '$dashfile'
  "
  [ "$status" -eq 0 ]
  [ "$output" = $'z' ]
}

@test "lines: trailing comma is invalid (exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    printf 'a\nb\n' | lines 1, >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "lines: double comma is invalid (exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    printf 'a\nb\n' | lines 1,,2 >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "lines: whitespace around comma is allowed" {
  run_lines "'1, 3'" $'a\nb\nc\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nc' ]
}

@test "lines: whitespace around .. is allowed (1 .. 2)" {
  run_lines "'1 .. 2'" $'a\nb\nc\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nb' ]
}

@test "lines: whitespace around open-start .. is allowed (' .. 2 ')" {
  run_lines "' .. 2 '" $'a\nb\nc\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nb' ]
}

@test "lines: file open error is runtime error (exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    lines 1 no_such_file >/dev/null
  "
  [ "$status" -eq 2 ]
}
