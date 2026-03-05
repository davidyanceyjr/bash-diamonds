#!/usr/bin/env bats
# tests/spec_cli_docs.bats

@test "cli docs: positional-primary contract wording is present" {
  run bash -c '
    set -euo pipefail
    docs=(lines fields match take trim table count filter replace)
    for name in "${docs[@]}"; do
      file="docs/${name}.md"
      [ -f "$file" ]
      grep -Fq "Primary form is positional:" "$file"
      grep -Eq "^## Synopsis$" "$file"
      grep -Eq "^[[:space:]]*${name} --help([[:space:]]*)$" "$file"
    done
  '
  [ "$status" -eq 0 ]
}

@test "cli docs: strict option parsing wording is present" {
  run bash -c '
    set -euo pipefail
    docs=(lines fields match take trim table count filter replace)
    for name in "${docs[@]}"; do
      file="docs/${name}.md"
      grep -Eiq "`--help`.*exit[s]?[[:space:]]+0" "$file"
      grep -Eiq "`--`.*end[s]?[[:space:]]+option[[:space:]]+parsing" "$file"
      grep -Eiq "unknown option[s]?[[:space:]]+before[[:space:]]+`--`.*(exit|exits)[[:space:]]+`?2`?" "$file"
    done
  '
  [ "$status" -eq 0 ]
}
