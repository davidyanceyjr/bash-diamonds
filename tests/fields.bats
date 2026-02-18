#!/usr/bin/env bats
# tests/fields.bats

setup() {
  ROOT="${BATS_TEST_DIRNAME}/.."
  FIELDS_SO="${FIELDS_SO:-$ROOT/build/fields.debug.so}"

  if [[ ! -f "$FIELDS_SO" ]]; then
    echo "missing fields so: $FIELDS_SO" >&2
    return 2
  fi

  TMPDIR="${BATS_TEST_TMPDIR:-/tmp}"
  F1="$TMPDIR/fields_f1.txt"
  F2="$TMPDIR/fields_f2.txt"
}

# === ANCHOR:HELPERS-BEGIN ===
# Helper notes:
# - Pass FIELDS_SO as a positional parameter into the inner bash; do NOT rely on
#   unexported BATS variables being visible inside `bash -c`.
# - Pass SPEC and input as positionals to preserve embedded spaces in SPEC.
# - Use pipefail in the od helper so pipeline status reflects `fields`.
run_fields() {
  local spec="$1"
  local input="$2"
  run bash --noprofile --norc -c '
    so="$1"; spec="$2"; in="$3"
    enable -f "$so" fields || exit 99
    printf "%s" "$in" | fields "$spec"
  ' _ "$FIELDS_SO" "$spec" "$input"
}

run_fields_od() {
  local spec="$1"
  local input="$2"
  run bash --noprofile --norc -c '
    set -o pipefail
    so="$1"; spec="$2"; in="$3"
    enable -f "$so" fields || exit 99
    printf "%s" "$in" | fields "$spec" | od -An -tx1
  ' _ "$FIELDS_SO" "$spec" "$input"
}
# === ANCHOR:HELPERS-END ===

@test "fields: basic selection (field 2)" {
  run_fields "2" $'a b c\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'b' ]
}

@test "fields: multiple indices 1,3" {
  run_fields "1,3" $'a b c\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a c' ]
}

@test "fields: closed range 2..3" {
  run_fields "2..3" $'a b c d\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'b c' ]
}

@test "fields: open start ..2" {
  run_fields "..2" $'a b c\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a b' ]
}

@test "fields: open end 2.." {
  run_fields "2.." $'a b c\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'b c' ]
}

@test "fields: dedup + ordering (3,1,1 => 1 then 3)" {
  run_fields "3,1,1" $'a b c\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a c' ]
}

@test "fields: overlapping ranges merge/dedupe (2..3,3..4)" {
  run_fields "2..3,3..4" $'a b c d e\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'b c d' ]
}

@test "fields: unsorted spec normalizes (3,1 => emits 1 then 3)" {
  run_fields "3,1" $'a b c\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a c' ]
}

@test "fields: whitespace collapsing (spaces + tabs)" {
  run_fields "1,3" $'\t a\t\t b   c \t\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a c' ]
}

@test "fields: empty/whitespace-only lines produce no output lines" {
  run_fields "1" $'\n   \t\nA B\n\t\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'A' ]
}

@test "fields: select beyond field-count everywhere => no output, exit 1" {
  run_fields "10" $'a b\nc d\n'
  [ "$status" -eq 1 ]
  [ "$output" = "" ]
}

@test "fields: partially beyond field-count (2..10) emits existing, exit 0" {
  run_fields "2..10" $'a b\nc d e\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'b\nd e' ]
}

@test "fields: newline preservation (unterminated last line)" {
  run_fields_od "2" $'a b\nX Y'
  [ "$status" -eq 0 ]
  # should contain: 'b' (62) then newline (0a) then 'Y' (59) with no trailing 0a
  [[ "$output" =~ (^|[[:space:]])62($|[[:space:]]) ]]
  [[ "$output" =~ (^|[[:space:]])0a($|[[:space:]]) ]]
  [[ "$output" =~ (^|[[:space:]])59($|[[:space:]]) ]]
  last=$(echo "$output" | awk '{print $NF}')
  [ "$last" != "0a" ]
}

@test "fields: newline preservation when last line emits nothing (unterminated input)" {
  run_fields_od "3" $'a b\nX Y'
  [ "$status" -eq 1 ]
  [ "$output" = "" ]
}

@test "fields: file concatenation + stdin in middle" {
  printf 'aa bb\n' >"$F1"
  printf 'mm nn\n' >"$F2"

  run bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    printf 'xx yy\n' | fields 1 '$F1' - '$F2'
  "
  [ "$status" -eq 0 ]
  [ "$output" = $'aa\nxx\nmm' ]
}

@test "fields: --help prints usage to stdout, exit 0" {
  run bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    fields --help
  "
  [ "$status" -eq 0 ]
  [[ "$output" =~ usage: ]]
}

@test "fields: missing SPEC => exit 2" {
  run bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    fields >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "fields: unknown -x token before -- is usage error (exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    printf 'a b\n' | fields -x >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "fields: '-' in FILE list denotes stdin at that position" {
  printf 'aa bb\n' >"$F1"

  run bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    printf 'xx yy\n' | fields 2 '$F1' -
  "
  [ "$status" -eq 0 ]
  [ "$output" = $'bb\nyy' ]
}

@test "fields: '--' ends option parsing; -name after -- is treated as filename (missing => exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    fields 1 -- -no_such_file >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "fields: '--' allows dash-leading filenames (existing file succeeds)" {
  local dashfile="$TMPDIR/-dashfile-fields"
  printf 'z y\n' >"$dashfile"

  run bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    fields 1 -- '$dashfile'
  "
  [ "$status" -eq 0 ]
  [ "$output" = $'z' ]
}

@test "fields: file open error is runtime error (exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    fields 1 /no/such/file >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "fields: stdout write error is runtime error (exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    exec 1>&-          # ANCHOR:STDOUT-CLOSE
    printf 'a b\n' | fields 1
  "
  [ "$status" -eq 2 ]
}

@test "fields: reversed range (3..1) => exit 2" {
  run_fields "3..1" $'a b c\n'
  [ "$status" -eq 2 ]
}

@test "fields: leading zero (01) => exit 2" {
  run_fields "01" $'a b\n'
  [ "$status" -eq 2 ]
}

@test "fields: bare '..' => exit 2" {
  run_fields ".." $'a b\n'
  [ "$status" -eq 2 ]
}

@test "fields: uint64 overflow => exit 2" {
  run_fields "18446744073709551616" $'a b\n'
  [ "$status" -eq 2 ]
}

@test "fields: trailing comma is invalid (exit 2)" {
  run_fields "1," $'a b\n'
  [ "$status" -eq 2 ]
}

@test "fields: double comma is invalid (exit 2)" {
  run_fields "1,,2" $'a b\n'
  [ "$status" -eq 2 ]
}

@test "fields: whitespace around comma is allowed" {
  run_fields "1 , 2" $'a b\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a b' ]
}

@test "fields: whitespace around .. is allowed (1 .. 2)" {
  run_fields "1 .. 2" $'a b c\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a b' ]
}

@test "fields: whitespace around open-start .. is allowed (' .. 2 ')" {
  run_fields " .. 2 " $'a b c\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'a b' ]
}
