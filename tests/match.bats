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
  [ "$output" = $'alpha\nbeta\ngamma' ]
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

@test "match: dot wildcard matches any single char (non-empty lines)" {
  run bash_with_match 'printf "a\n\nbc\n" | match "."'
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nbc' ]
}

@test "match: question mark makes previous atom optional" {
  run bash_with_match 'printf "color\ncolour\ncolouur\n" | match "^colou?r$"'
  [ "$status" -eq 0 ]
  [ "$output" = $'color\ncolour' ]
}

@test "match: plus requires one-or-more" {
  run bash_with_match 'printf "a\n\nbbb\n" | match "^b+$"'
  [ "$status" -eq 0 ]
  [ "$output" = "bbb" ]
}

@test "match: star allows zero-or-more (matches empty line too)" {
  # If your spec forbids empty matches, change this test accordingly.
  run bash_with_match 'printf "\n\naaa\nbbb\n" | match "^a*$"'
  [ "$status" -eq 0 ]
  [ "$output" = $'\n\naaa' ]
}

@test "match: grouping with alternation precedence" {
  run bash_with_match 'printf "ac\nabc\nabbc\n" | match "^(ab|a)c$"'
  [ "$status" -eq 0 ]
  [ "$output" = $'ac\nabc' ]
}

@test "match: alternation basic" {
  run bash_with_match 'printf "foo\nbar\nbaz\nqux\n" | match "^(foo|baz)$"'
  [ "$status" -eq 0 ]
  [ "$output" = $'foo\nbaz' ]
}

@test "match: character class simple" {
  run bash_with_match 'printf "a\nb\nc\nd\n" | match "^[a-c]$"'
  [ "$status" -eq 0 ]
  [ "$output" = $'a\nb\nc' ]
}

@test "match: character class negation" {
  run bash_with_match 'printf "a\nb\nc\n" | match "^[^a]$"'
  [ "$status" -eq 0 ]
  [ "$output" = $'b\nc' ]
}

@test "match: escapes treat metacharacters literally" {
  run bash_with_match 'printf "a.c\nabc\na-c\n" | match "^a\\.c$"'
  [ "$status" -eq 0 ]
  [ "$output" = "a.c" ]
}

@test "match: unterminated final line is emitted unterminated" {
  tmp="$BATS_TEST_TMPDIR/match_bytes_$$.out"

  # final line has no trailing newline
  bash_with_match 'printf "alpha\nbeta" | match beta > "'"$tmp"'"'
  status=$?
  [ "$status" -eq 0 ]

  bytes="$(od -An -tx1 "$tmp" | tr -d " \n")"
  [ "$bytes" = "62657461" ]  # "beta"
}

@test "match: CRLF bytes are preserved (\\r is part of line bytes)" {
  tmp="$BATS_TEST_TMPDIR/match_crlf_$$.out"

  # Use $'...' so the pattern contains an actual 0x0d byte.
  bash_with_match 'printf $'"'"'alpha\r\nbeta\r\ngamma\r\n'"'"' | match $'"'"'^beta\r$'"'"' > "'"$tmp"'"'
  status=$?
  [ "$status" -eq 0 ]

  bytes="$(od -An -tx1 "$tmp" | tr -d " \n")"
  # "beta\r\n" => 62 65 74 61 0d 0a
  [ "$bytes" = "626574610d0a" ]
}

@test "match: option parsing is strict (unknown -x before -- is usage error)" {
  run bash_with_match 'printf "a\n" | match -v a 2>&1'
  [ "$status" -eq 2 ]
  [[ "$output" == match:* ]]
}

@test "match: pattern beginning with '-' requires --" {
  run bash_with_match 'printf -- "-foo\nbar\n" | match -- "-foo"'
  [ "$status" -eq 0 ]
  [ "$output" = "-foo" ]
}

@test "match: invalid regex (unclosed [) is usage/runtime error (exit 2) with one-line message" {
  run bash_with_match 'printf "a\n" | match "[" 2>&1'
  [ "$status" -eq 2 ]
  [[ "$output" == match:* ]]
  [[ "$output" != *$'\n'* ]]
}

@test "match: invalid regex (unclosed () is usage/runtime error (exit 2) with one-line message" {
  run bash_with_match 'printf "a\n" | match "(" 2>&1'
  [ "$status" -eq 2 ]
  [[ "$output" == match:* ]]
  [[ "$output" != *$'\n'* ]]
}

@test "match: invalid regex (dangling escape) is usage/runtime error (exit 2) with one-line message" {
  run bash_with_match 'printf "a\n" | match "\\" 2>&1'
  [ "$status" -eq 2 ]
  [[ "$output" == match:* ]]
  [[ "$output" != *$'\n'* ]]
}

@test "match: stdout write error maps to exit 2 (SIGPIPE ignored internally)" {
  run bash -c '
    set -o pipefail
    enable -f "$BASH_BUILTINS_DIR/match.debug.so" match
    awk "BEGIN{for(i=0;i<200000;i++) print \"x\"}" | match "^x$" | head -n1 >/dev/null
  '
  [ "$status" -eq 2 ]
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
