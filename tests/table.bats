#!/usr/bin/env bats
# tests/table.bats

setup() {
  ROOT="${BATS_TEST_DIRNAME}/.."
  TABLE_SO="${TABLE_SO:-$ROOT/build/table.debug.so}"

  if [[ ! -f "$TABLE_SO" ]]; then
    echo "missing table so: $TABLE_SO" >&2
    return 2
  fi

  TMPDIR="${BATS_TEST_TMPDIR:-/tmp}"
  F1="$TMPDIR/table_f1.txt"
  F2="$TMPDIR/table_f2.txt"
}

bash_with_table() {
  local cmd="$1"
  bash --noprofile --norc -c "
    enable -f '$TABLE_SO' table || exit 99
    $cmd
  "
}

@test "table: --help prints usage to stdout, exit 0" {
  run bash_with_table 'table --help'
  [ "$status" -eq 0 ]
  [[ "$output" =~ ^usage:\ table ]]
}

@test "table: unknown -x token before -- is usage error (exit 2)" {
  run bash_with_table 'table -x >/dev/null'
  [ "$status" -eq 2 ]
}

@test "table: formatting aligns columns and writes no trailing spaces" {
  printf 'a   bb   c\naaa b    ccc\n' >"$F1"

  run bash_with_table "table '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = $'a   bb  c\naaa b   ccc' ]

  # Ensure no line ends with a space.
  run bash_with_table "table '$F1' | awk '{print \$0}' | grep -nE ' +$' || true"
  [ "$status" -eq 0 ]
  [ "$output" = "" ]
}

@test "table: empty/whitespace-only input emits nothing (exit 1)" {
  printf '   \n\t\t\n\n' >"$F1"

  run bash_with_table "table '$F1'"
  [ "$status" -eq 1 ]
  [ "$output" = "" ]
}

@test "table: newline preservation; unterminated final line does not gain newline" {
  # Second line has no trailing newline.
  printf 'a b\nx y' >"$F1"

  run bash_with_table "table '$F1' | od -An -tx1"
  [ "$status" -eq 0 ]

  # Expect tokens: 61 20 62 0a 78 20 79 (no trailing 0a)
  [[ "$output" =~ (^|[[:space:]])61($|[[:space:]]) ]]
  [[ "$output" =~ (^|[[:space:]])62($|[[:space:]]) ]]
  [[ "$output" =~ (^|[[:space:]])0a($|[[:space:]]) ]]
  [[ "$output" =~ (^|[[:space:]])78($|[[:space:]]) ]]
  [[ "$output" =~ (^|[[:space:]])79($|[[:space:]]) ]]

  last="$(echo "$output" | awk '{print $NF}')"
  [ "$last" != "0a" ]
}

@test "table: stdin is rejected as non-seekable (exit 2 + exact message)" {
  run bash --noprofile --norc -c "
    enable -f '$TABLE_SO' table || exit 99
    printf 'a b\n' | table 2>&1 >/dev/null
  "
  [ "$status" -eq 2 ]
  [ "$output" = 'table: non-seekable input not supported' ]
}

@test "table: '-' argument is rejected as non-seekable (exit 2 + exact message)" {
  run bash --noprofile --norc -c "
    enable -f '$TABLE_SO' table || exit 99
    table - 2>&1 >/dev/null
  "
  [ "$status" -eq 2 ]
  [ "$output" = 'table: non-seekable input not supported' ]
}

@test "table: stdout write error is runtime error (exit 2)" {
  printf 'a b\n' >"$F1"

  run bash --noprofile --norc -c "
    enable -f '$TABLE_SO' table || exit 99
    exec 1>&-          # ANCHOR:STDOUT-CLOSE
    table '$F1'
  "
  [ "$status" -eq 2 ]
}