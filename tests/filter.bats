#!/usr/bin/env bats

# Enable the loadable builtin in each test shell.
bash_with_filter() {
  bash -c '
    set -e
    enable -f "$BASH_BUILTINS_DIR/filter.debug.so" filter
    '"$1"'
  '
}

@test "filter: --help exits 0 and begins with usage:" {
  run bash_with_filter 'filter --help'
  [ "$status" -eq 0 ]
  [[ "$output" =~ ^usage:\ filter\  ]]
}

@test "filter: option parsing is strict (unknown -x before -- is usage error)" {
  run bash_with_filter 'printf "a\n" | filter -x "\$1 == \"a\"" 2>&1'
  [ "$status" -eq 2 ]
  [[ "$output" == filter:* ]]
}

@test "filter: -- allows EXPR token beginning with '-'" {
  run bash_with_filter 'printf "x\n" | filter -- "-1 == -1"'
  [ "$status" -eq 0 ]
  [ "$output" = "x" ]
}

@test "filter: missing EXPR is a usage error (exit 2)" {
  run bash_with_filter 'filter 2>&1'
  [ "$status" -eq 2 ]
  [[ "$output" == filter:* ]]
}

@test "filter: basic string equality on field $1" {
  run bash_with_filter 'printf "ERROR\tfoo\nOK\tbar\n" | filter "\$1 == \"ERROR\""'
  [ "$status" -eq 0 ]
  [ "$output" = $'ERROR\tfoo' ]
}

@test "filter: numeric comparison when both sides parse as signed integers" {
  run bash_with_filter 'printf "a\t10\nb\t2\n" | filter "\$2 > 5"'
  [ "$status" -eq 0 ]
  [ "$output" = $'a\t10' ]
}

@test "filter: missing field yields empty string" {
  run bash_with_filter 'printf "a\n" | filter "\$2 == \"\""'
  [ "$status" -eq 0 ]
  [ "$output" = "a" ]
}

@test "filter: no matches returns exit 1" {
  run bash_with_filter 'printf "a\n" | filter "\$1 == \"z\""'
  [ "$status" -eq 1 ]
  [ -z "$output" ]
}

@test "filter: multi-file concatenation supports '-' in the middle" {
  f1="$BATS_TEST_TMPDIR/filter_f1_$$.txt"
  f2="$BATS_TEST_TMPDIR/filter_f2_$$.txt"
  printf "a\n" >"$f1"
  printf "c\n" >"$f2"

  run bash_with_filter 'printf "b\n" | filter "\$1 == \"b\"" "'"$f1"'" - "'"$f2"'"'
  [ "$status" -eq 0 ]
  [ "$output" = "b" ]
}

@test "filter: unterminated final line is emitted unterminated" {
  tmp="$BATS_TEST_TMPDIR/filter_bytes_$$.out"
  bash_with_filter 'printf "a\nkeep" | filter "\$1 == \"keep\"" > "'"$tmp"'"'
  status=$?
  [ "$status" -eq 0 ]

  bytes="$(od -An -tx1 "$tmp" | tr -d " \n")"
  [ "$bytes" = "6b656570" ]  # "keep"
}

@test "filter: stdout write error maps to exit 2 (SIGPIPE ignored internally)" {
  run bash -c '
    set -o pipefail
    enable -f "$BASH_BUILTINS_DIR/filter.debug.so" filter
    awk "BEGIN{for(i=0;i<200000;i++) print \"x\"}" | filter "\$1 == \"x\"" | head -n1 >/dev/null
  '
  [ "$status" -eq 2 ]
}