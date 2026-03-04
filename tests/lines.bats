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

# === ANCHOR:HELPERS-BEGIN ===
bash_with_lines() {
  local cmd="$1"
  bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    $cmd
  "
}
# === ANCHOR:HELPERS-END ===

@test "lines: usage error on no args" {
  run bash_with_lines 'lines'
  [ "$status" -eq 2 ]
}

@test "lines: select single line" {
  printf "a\nb\nc\n" >"$F1"

  run bash_with_lines "lines 2 '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = $'b' ]
}

@test "lines: select multiple lines by list" {
  printf "a\nb\nc\nd\n" >"$F1"

  run bash_with_lines "lines 1,3 '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nc' ]
}

@test "lines: select range 2..3" {
  printf "a\nb\nc\nd\n" >"$F1"

  run bash_with_lines "lines 2..3 '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = $'b\nc' ]
}

@test "lines: open start ..2" {
  printf "a\nb\nc\nd\n" >"$F1"

  run bash_with_lines "lines ..2 '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nb' ]
}

@test "lines: open end 3.." {
  printf "a\nb\nc\nd\n" >"$F1"

  run bash_with_lines "lines 3.. '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = $'c\nd' ]
}

@test "lines: selection across multiple files is concatenated" {
  printf "a\nb\n" >"$F1"
  printf "c\nd\n" >"$F2"

  run bash_with_lines "lines 3 '$F1' '$F2'"
  [ "$status" -eq 0 ]
  [ "$output" = $'c' ]
}

@test "lines: no result returns 1" {
  printf "a\n" >"$F1"

  run bash_with_lines "lines 9 '$F1'"
  [ "$status" -eq 1 ]
  [ "$output" = "" ]
}

@test "lines: preserves missing trailing newline on final line" {
  # file ends without '\n'
  printf "a\nb" >"$F1"

  run bash_with_lines "lines 2 '$F1'"
  [ "$status" -eq 0 ]
  [ "$output" = $'b' ]
}

@test "lines: --help prints usage to stdout, exit 0" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    lines --help
  "
  [ "$status" -eq 0 ]
  [[ "$output" =~ ^usage:\ lines ]]
}

@test "lines: dash-leading filename works without --" {
  local dashf="$TMPDIR/-dashfile"
  printf "x\ny\n" >"$dashf"

  run bash_with_lines "lines 1 '$dashf'"
  [ "$status" -eq 0 ]
  [ "$output" = $'x' ]
}

@test "lines: -- allows dash-leading filename" {
  local dashf="$TMPDIR/-dashfile"
  printf "x\ny\n" >"$dashf"

  run bash_with_lines "lines 1 -- '$dashf'"
  [ "$status" -eq 0 ]
  [ "$output" = $'x' ]
}

@test "lines: stdout write failure exits 2 (SIGPIPE ignored)" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    exec 1>&-
    printf 'a\n' | lines 1
  "
  [ "$status" -eq 2 ]
}
