#!/usr/bin/env bats

# Enable the loadable builtin in each test shell.
# We do it inside `bash -c` so the pipeline uses a shell that has `take` enabled.
bash_with_take() {
  bash -c '
    set -e
    enable -f "$BASH_BUILTINS_DIR/take.debug.so" take
    '"$1"'
  '
}

@test "take: --help prints usage and exits 0" {
  run bash_with_take 'take --help'
  [ "$status" -eq 0 ]
  [[ "$output" == *"take N"* ]]
}

@test "take: missing N => exit 2" {
  run bash_with_take 'take 2>&1'
  [ "$status" -eq 2 ]
  [[ "$output" == take:* ]]
}

@test "take: take 3 on 5-line input => 3 lines, exit 0" {
  run bash_with_take 'printf "1\n2\n3\n4\n5\n" | take 3'
  [ "$status" -eq 0 ]
  [ "$output" = $'1\n2\n3' ]
}

@test "take: take 3 on 2-line input => 2 lines, exit 0" {
  run bash_with_take 'printf "a\nb\n" | take 3'
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nb' ]
}

@test "take: take 0 on non-empty input => outputs nothing, exit 1" {
  run bash_with_take 'printf "a\n" | take 0'
  [ "$status" -eq 1 ]
  [ -z "$output" ]
}

@test "take: take 2 1 on 5-line input => outputs lines 2-3, exit 0" {
  run bash_with_take 'printf "1\n2\n3\n4\n5\n" | take 2 1'
  [ "$status" -eq 0 ]
  [ "$output" = $'2\n3' ]
}

@test "take: take 2 10 on 5-line input => outputs nothing, exit 1" {
  run bash_with_take 'printf "1\n2\n3\n4\n5\n" | take 2 10'
  [ "$status" -eq 1 ]
  [ -z "$output" ]
}

@test "take: unterminated final line preserved (no newline added)" {
  tmp="$BATS_TEST_TMPDIR/take_bytes_$$.out"
  run bash_with_take 'printf "alpha\nbeta" | take 1 1 > "'"$tmp"'"'
  [ "$status" -eq 0 ]

  # Expect exactly "beta" (hex: 62 65 74 61) and no 0a
  bytes="$(od -An -tx1 "$tmp" | tr -d " \n")"
  [ "$bytes" = "62657461" ]
}

@test "take: invalid numerics (+1, -1, 01, 00, 1x) => exit 2" {
  for n in +1 -1 01 00 1x; do
    run bash_with_take "printf 'a\n' | take $n 2>&1"
    [ "$status" -eq 2 ]
  done
}

@test "take: unsupported option before -- (-n 3, --foo) => exit 2" {
  run bash_with_take 'printf "a\n" | take -n 3 2>&1'
  [ "$status" -eq 2 ]
  [[ "$output" == take:* ]]

  run bash_with_take 'printf "a\n" | take --foo 3 2>&1'
  [ "$status" -eq 2 ]
  [[ "$output" == take:* ]]
}