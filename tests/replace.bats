#!/usr/bin/env bats

@test "replace --help exits 0 and begins with usage:" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    replace --help
  '
  [ "$status" -eq 0 ]
  [[ "$output" == usage:\ replace\ * ]]
}

@test "replace: unknown -x before -- is usage error (exit 2)" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    printf "a\n" | replace -x foo bar
  '
  [ "$status" -eq 2 ]
}

@test "replace: missing arguments is usage error (exit 2)" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    printf "a\n" | replace foo
  '
  [ "$status" -eq 2 ]
}

@test "replace: empty PATTERN is usage error (exit 2)" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    printf "a\n" | replace "" x
  '
  [ "$status" -eq 2 ]
}

@test "replace: duplicate --literal is usage error (exit 2)" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    printf "a\n" | replace --literal --literal a x
  '
  [ "$status" -eq 2 ]
}

@test "replace: invalid regex is runtime error (exit 2)" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    printf "a\n" | replace "[" x
  '
  [ "$status" -eq 2 ]
}

@test "replace: literal mode basic replacement" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    printf "foo\n" | replace --literal foo bar
  '
  [ "$status" -eq 0 ]
  [ "$output" = "bar" ]
}

@test "replace: regex mode basic replacement" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    printf "foo\n" | replace "f.o" bar
  '
  [ "$status" -eq 0 ]
  [ "$output" = "bar" ]
}

@test "replace: multiple replacements per line" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    printf "a a a\n" | replace --literal a x
  '
  [ "$status" -eq 0 ]
  [ "$output" = "x x x" ]
}

@test "replace: no substitution -> exit 1 and no output" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    printf "foo\n" | replace --literal bar baz
  '
  [ "$status" -eq 1 ]
  [ "$output" = "" ]
}

@test "replace: unterminated final line preserved (no trailing newline emitted)" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    printf "foo" | replace --literal foo bar
  '
  [ "$status" -eq 0 ]
  [ "$output" = "bar" ]
}

@test "replace: file concatenation (only modified lines from both files)" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    t1=$(mktemp); t2=$(mktemp)
    printf "a\nb\n" >"$t1"
    printf "b\nc\n" >"$t2"
    replace --literal b x "$t1" "$t2"
    rc=$?
    rm -f "$t1" "$t2"
    exit $rc
  '
  [ "$status" -eq 0 ]
  [ "$output" = $'x\nx' ]
}

@test "replace: '-' in middle is stdin and processed in order" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    t1=$(mktemp); t2=$(mktemp)
    printf "a\n" >"$t1"
    printf "a\n" >"$t2"
    printf "a\n" | replace --literal a x "$t1" - "$t2"
    rc=$?
    rm -f "$t1" "$t2"
    exit $rc
  '
  [ "$status" -eq 0 ]
  [ "$output" = $'x\nx\nx' ]
}

@test "replace: PATTERN beginning with '-' requires '--'" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    # Use a dash-leading PATTERN that is not the option terminator token itself.
    printf "%s\n" -- "-x" | replace --literal -- -x :
  '
  [ "$status" -eq 0 ]
  [ "$output" = ":" ]
}

@test "replace: stdout write failure exits 2 (SIGPIPE ignored)" {
  run bash -c '
    enable -f "$BASH_BUILTINS_DIR/replace.debug.so" replace
    exec 1>&-         # close stdout
    printf "foo\n" | replace --literal foo bar
  '
  [ "$status" -eq 2 ]
}
