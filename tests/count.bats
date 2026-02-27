#!/usr/bin/env bats
# tests/count.bats

setup() {
  ROOT="${BATS_TEST_DIRNAME}/.."
  COUNT_SO="${COUNT_SO:-$ROOT/build/count.debug.so}"

  if [[ ! -f "$COUNT_SO" ]]; then
    echo "missing count so: $COUNT_SO" >&2
    return 2
  fi

  TMPDIR="${BATS_TEST_TMPDIR:-/tmp}"
  F1="$TMPDIR/count_f1.txt"
  F2="$TMPDIR/count_f2.txt"
}

# === ANCHOR:HELPERS-BEGIN ===
bash_with_count() {
  local cmd="$1"
  bash --noprofile --norc -c "
    enable -f '$COUNT_SO' count || exit 99
    $cmd
  "
}
# === ANCHOR:HELPERS-END ===

@test "count --help exits 0 and begins with usage:" {
  run bash_with_count 'count --help'
  [ "$status" -eq 0 ]
  [[ "$output" == usage:* ]]
}

@test "count: unknown -x before -- is usage error (exit 2)" {
  run bash_with_count 'count -x'
  [ "$status" -eq 2 ]
}

@test "count: empty input outputs 0 and exits 0" {
  run bash_with_count "printf '' | count"
  [ "$status" -eq 0 ]
  [ "$output" = "0" ]
}

@test "count: simple stdin count" {
  run bash_with_count "printf 'a\nb\n' | count"
  [ "$status" -eq 0 ]
  [ "$output" = "2" ]
}

@test "count: unterminated final line counts" {
  run bash_with_count "printf 'a\nb' | count"
  [ "$status" -eq 0 ]
  [ "$output" = "2" ]
}

@test "count: file count" {
  printf "a\nb\n" >"$F1"
  run bash_with_count "count '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = "2" ]
}

@test "count: multiple files concatenate" {
  printf "a\n" >"$F1"
  printf "b\n" >"$F2"
  run bash_with_count "count '$F1' '$F2'"
  [ "$status" -eq 0 ]
  [ "$output" = "2" ]
}

@test "count: '-' in middle is stdin and processed in order" {
  printf "a\n" >"$F1"
  run bash_with_count "printf 'b\n' | count '$F1' -"
  [ "$status" -eq 0 ]
  [ "$output" = "2" ]
}

@test "count: stdout write failure exits 2 (SIGPIPE ignored)" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/count.debug.so" count
    exec 1>&-         # close stdout
    count </dev/null
  '
  [ "$status" -eq 2 ]
}
