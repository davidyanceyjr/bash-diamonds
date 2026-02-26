@test "lines: --help prints usage to stdout, exit 0" {
  run bash --noprofile --norc -c "
    enable -f '$LINES_SO' lines || exit 99
    lines --help
  "
  [ "$status" -eq 0 ]
  [[ "$output" =~ ^usage:\ lines\ SPEC ]]
  [[ "$output" =~ Options: ]]
  [[ "$output" =~ --help ]]
  [[ "$output" =~ Exit\ codes: ]]
  [[ "$output" =~ "  0" ]]
  [[ "$output" =~ "  1" ]]
  [[ "$output" =~ "  2" ]]
  # lines must not claim to support -d
  [[ ! "$output" =~ "-d" ]]
}