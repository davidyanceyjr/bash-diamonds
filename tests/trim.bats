#!/usr/bin/env bats

# tests/trim.bats

setup() {
  ROOT="${BATS_TEST_DIRNAME}/.."
  TRIM_SO="${TRIM_SO:-$ROOT/build/trim.debug.so}"

  if [[ ! -f "$TRIM_SO" ]]; then
    echo "missing trim so: $TRIM_SO" >&2
    return 2
  fi

  TMPDIR="${BATS_TEST_TMPDIR:-/tmp}"
  F1="$TMPDIR/trim_f1.txt"
  F2="$TMPDIR/trim_f2.txt"
}

run_trim() {
  local input="$1"
  run bash --noprofile --norc -c "
    enable -f '$TRIM_SO' trim || exit 99
    printf '%s' \"$input\" | trim
  "
}

run_trim_od() {
  local input="$1"
  run bash --noprofile --norc -c "
    enable -f '$TRIM_SO' trim || exit 99
    printf '%s' \"$input\" | trim | od -An -tx1
  "
}

@test "trim: basic trimming (spaces/tabs/cr/vt/ff)" {
  run_trim $'  alpha  \n\tbeta\t\n\rgamma\v\f\n'
  [ "$status" -eq 0 ]
  [ "$output" = $'alpha\nbeta\ngamma' ]
}

@test "trim: whitespace-only lines emit nothing" {
  run_trim $'   \n\t\t\n\r\v\f\n'
  [ "$status" -eq 1 ]
  [ "$output" = "" ]
}

@test "trim: newline preservation; unterminated final line does not gain newline" {
  run_trim_od $'  a  \n  b  '
  [ "$status" -eq 0 ]
  # Expect: 'a' (61) '\n' (0a) 'b' (62) and no trailing 0a
  [[ "$output" =~ (^|[[:space:]])61($|[[:space:]]) ]]
  [[ "$output" =~ (^|[[:space:]])0a($|[[:space:]]) ]]
  [[ "$output" =~ (^|[[:space:]])62($|[[:space:]]) ]]
  # Ensure the last non-whitespace token isn't 0a (no trailing newline)
  last="$(echo "$output" | awk '{print $NF}')"
  [ "$last" != "0a" ]
}

@test "trim: file concatenation + stdin in middle" {
  printf '  a  \n' >"$F1"
  printf '  c  \n' >"$F2"

  run bash --noprofile --norc -c "
    enable -f '$TRIM_SO' trim || exit 99
    printf '  b  \n' | trim '$F1' - '$F2'
  "
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nb\nc' ]
}

@test "trim: --help prints usage to stdout, exit 0" {
  run bash --noprofile --norc -c "
    enable -f '$TRIM_SO' trim || exit 99
    trim --help
  "
  [ "$status" -eq 0 ]
  [[ "$output" =~ usage: ]]
}

@test "trim: unknown -x token before -- is usage error (exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$TRIM_SO' trim || exit 99
    printf 'a\n' | trim -x >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "trim: '--' ends option parsing; -name after -- is treated as filename (missing => exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$TRIM_SO' trim || exit 99
    trim -- -no_such_file >/dev/null
  "
  [ "$status" -eq 2 ]
}

@test "trim: exit code 1 when nothing emitted" {
  run_trim $'\n  \n\t\n'
  [ "$status" -eq 1 ]
  [ "$output" = "" ]
}

@test "trim: stdout write error is runtime error (exit 2)" {
  run bash --noprofile --norc -c "
    enable -f '$TRIM_SO' trim || exit 99
    exec 1>&-          # ANCHOR:STDOUT-CLOSE
    printf 'a\n' | trim
  "
  [ "$status" -eq 2 ]
}
