#!/usr/bin/env bats
# tests/spec_inventory.bats

@test "spec inventory: stable v1 list remains the expected set" {
  run bash -c '
    set -euo pipefail
    actual="$(
      awk '"'"'
        /^### 3\.1 Stable \(v1\.0 Contract\)$/ { in_section=1; next }
        in_section && /^### / { in_section=0 }
        in_section && /^- / { sub(/^- /, "", $0); print }
      '"'"' docs/project-spec.md
    )"
    expected=$'"'"'lines\nfields\nmatch\ntake\ntrim\ntable\ncount\nfilter'"'"'
    [ "$actual" = "$expected" ]
  '
  [ "$status" -eq 0 ]
}

@test "spec inventory: no planned builtins and non-contract implemented list is explicit" {
  run bash -c '
    set -euo pipefail
    planned_body="$(awk '"'"'
      /^### 3\.3 Planned$/ { in_section=1; next }
      in_section && /^### / { in_section=0 }
      in_section { print }
    '"'"' docs/project-spec.md)"
    printf "%s\n" "$planned_body" | grep -qx "None."
    ! printf "%s\n" "$planned_body" | grep -q "^- "

    non_contract="$(
      awk '"'"'
        /^### 3\.4 Implemented but Non-Contract$/ { in_section=1; next }
        in_section && /^### / { in_section=0 }
        in_section && /^- / { sub(/^- /, "", $0); print }
      '"'"' docs/project-spec.md
    )"
    expected=$'"'"'freq\nalone\narrange'"'"'
    [ "$non_contract" = "$expected" ]
  '
  [ "$status" -eq 0 ]
}

@test "spec inventory: implemented non-contract tools have builtin sources" {
  run bash -c '
    set -euo pipefail
    for name in freq alone arrange; do
      [ -f "src/builtins/builtin_${name}.c" ]
    done
  '
  [ "$status" -eq 0 ]
}
