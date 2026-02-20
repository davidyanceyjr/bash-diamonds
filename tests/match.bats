#!/usr/bin/env bats

# Enable the loadable builtin in each test shell.
# We do it inside `bash -c` so the pipeline uses a shell that has `match` enabled.
bash_with_match() {
  bash -c '
    set -e
    enable -f "$BASH_BUILTINS_DIR/match.debug.so" match
    '"$1"'
  '
}

@test "match: basic substring match (default keep)" {
  run bash_with_match 'printf "alpha\nbeta\ngamma\n" | match a'
  [ "$status" -eq 0 ]
  [ "$output" = $'alpha\ngamma' ]
}

@test "match: invert match (-v)" {
  run bash_with_match 'printf "alpha\nbeta\ngamma\n" | match -v a'
  [ "$status" -eq 0 ]
  [ "$output" = "beta" ]
}

@test "match: no matches returns exit 1" {
  run bash_with_match 'printf "alpha\nbeta\n" | match z'
  [ "$status" -eq 1 ]
  [ -z "$output" ]
}

@test "match: anchor ^ works" {
  run bash_with_match 'printf "foo\nbar\nbaz\n" | match "^ba"'
  [ "$status" -eq 0 ]
  [ "$output" = $'bar\nbaz' ]
}

@test "match: anchor $ works" {
  run bash_with_match 'printf "foo\nbar\nbaz\n" | match "ar$"'
  [ "$status" -eq 0 ]
  [ "$output" = "bar" ]
}

@test "match: -n prefixes line numbers" {
  run bash_with_match 'printf "a\nb\na\n" | match -n a'
  [ "$status" -eq 0 ]
  [ "$output" = $'1:a\n3:a' ]
}

@test "match: -c counts matches" {
  run bash_with_match 'printf "a\nb\na\n" | match -c a'
  [ "$status" -eq 0 ]
  [ "$output" = "2" ]
}

@test "match: -c with no matches returns 1 and prints 0" {
  run bash_with_match 'printf "a\nb\n" | match -c z'
  [ "$status" -eq 1 ]
  [ "$output" = "0" ]
}

@test "match: no trailing newline on single match" {
  tmp="$BATS_TEST_TMPDIR/match_bytes_$$.out"

  bash_with_match 'printf "alpha\nbeta\n" | match beta > "'"$tmp"'"'
  status=$?
  [ "$status" -eq 0 ]

  bytes="$(od -An -tx1 "$tmp" | tr -d " \n")"
  [ "$bytes" = "62657461" ]  # "beta"
}

@test "match: large input streaming sanity" {
  tmp="$BATS_TEST_TMPDIR/match_large_$$.out"

  awk 'BEGIN { for (i=0;i<200000;i++) printf "a"; printf "\n" }' \
    | bash -c '
        : "${BASH_BUILTINS_DIR:?}"
        enable -f "$BASH_BUILTINS_DIR/match.debug.so" match
        match a > "'"$tmp"'"
      '

  status=$?
  [ "$status" -eq 0 ]

  size="$(wc -c < "$tmp" | tr -d " ")"
  [ "$size" -ge 200000 ]
}
