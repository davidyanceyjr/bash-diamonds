@test "fields: --help prints usage to stdout, exit 0" {
  run bash --noprofile --norc -c "
    enable -f '$FIELDS_SO' fields || exit 99
    fields --help
  "
  [ "$status" -eq 0 ]
  [[ "$output" =~ ^usage:\ fields\ \[-d\ DELIM\]\ SPEC ]]
  [[ "$output" =~ Options: ]]
  [[ "$output" =~ --help ]]
  [[ "$output" =~ "-d DELIM" ]]
  [[ "$output" =~ Exit\ codes: ]]
  [[ "$output" =~ "  0" ]]
  [[ "$output" =~ "  1" ]]
  [[ "$output" =~ "  2" ]]
}