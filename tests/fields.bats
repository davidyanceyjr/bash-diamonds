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
# - Pass FIELDS_SO explicitly to avoid PATH confusion.
# - Use a clean non-interactive bash.
bash_with_fields() {
  local cmd="$1"
  bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    $cmd
  "
}
# === ANCHOR:HELPERS-END ===

@test "fields: usage error on no args" {
  run bash_with_fields 'fields'
  [ "$status" -eq 2 ]
}

@test "fields: --help prints usage to stdout, exit 0" {
  run bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    fields --help
  "
  [ "$status" -eq 0 ]
  [[ "$output" == $'usage: fields SPEC [FILE...]\n'* ]]
}

@test "fields: unknown option before -- is usage error" {
  printf "a b\n" >"$F1"

  run bash_with_fields "fields -x 1 '$F1'"
  [ "$status" -eq 2 ]
}

@test "fields: -- after SPEC allows file operand beginning with '-'" {
  run bash_with_fields "cd '$TMPDIR' || exit 99; printf 'a b\\n' > -x; fields 1 -- -x"
  [ "$status" -eq 0 ]
  [ "$output" = $'a' ]
}

@test "fields: selects 1st field (whitespace mode)" {
  printf "a b c\nx  y\tz\n" >"$F1"

  run bash_with_fields "fields 1 '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nx' ]
}

@test "fields: selects 2nd and 3rd fields (whitespace mode) joined with space" {
  printf "a b c\nx  y\tz\n" >"$F1"

  run bash_with_fields "fields 2..3 '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = $'b c\ny z' ]
}

@test "fields: default join uses literal 0x20 space" {
  printf "a b c\n" >"$F1"

  run bash_with_fields "fields 1,3 '$F1' | od -An -tx1"
  [ "$status" -eq 0 ]
  [[ "$output" =~ 61[[:space:]]20[[:space:]]63[[:space:]]0a ]]
}

@test "fields: --tsv join uses literal 0x09 tab" {
  printf "a b c\n" >"$F1"

  run bash_with_fields "fields --tsv 1,3 '$F1' | od -An -tx1"
  [ "$status" -eq 0 ]
  [[ "$output" =~ 61[[:space:]]09[[:space:]]63[[:space:]]0a ]]
}

@test "fields: no result returns 1" {
  printf "a b\n" >"$F1"

  run bash_with_fields "fields 9 '$F1'"
  [ "$status" -eq 1 ]
  [ "$output" = "" ]
}

@test "fields: -d delimiter mode selects fields" {
  printf "u:x:1:2::/home/u:/bin/sh\n" >"$F1"

  run bash_with_fields "fields -d: 1,7 '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = $'u /bin/sh' ]
}

@test "fields: -d preserves empty fields between consecutive delimiters" {
  printf "a::c\n" >"$F1"

  run bash_with_fields "fields -d: 1..3 '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = $'a  c' ]
}

@test "fields: -d requires exactly 1 byte delimiter" {
  printf "a,b\n" >"$F1"

  run bash_with_fields "fields -d '' 1 '$F1'"
  [ "$status" -eq 2 ]
}

@test "fields: --tsv preserves newline determinism" {
  # With input newline: output newline preserved
  printf "a b\n" >"$F1"
  run bash_with_fields "fields --tsv 1..2 '$F1' | od -An -tx1"
  [ "$status" -eq 0 ]
  [[ "$output" =~ 61[[:space:]]09[[:space:]]62[[:space:]]0a ]]

  # Without input newline: no newline synthesized
  printf "a b" >"$F1"
  run bash_with_fields "fields --tsv 1..2 '$F1' | od -An -tx1"
  [ "$status" -eq 0 ]
  [[ "$output" =~ 61[[:space:]]09[[:space:]]62$ ]]
}

@test "fields: --tsv works across multiple files" {
  printf "a b\n" >"$F1"
  printf "x y\n" >"$F2"

  run bash_with_fields "fields --tsv 1..2 '$F1' '$F2'"
  [ "$status" -eq 0 ]
  [ "$output" = $'a\tb\nx\ty' ]
}