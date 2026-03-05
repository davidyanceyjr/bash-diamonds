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
    expected=$'"'"'lines\nfields\nmatch\ntake\ntrim\ntable\ncount\nfilter\nreplace'"'"'
    [ "$actual" = "$expected" ]
  '
  [ "$status" -eq 0 ]
}

@test "spec inventory: preview/planned/non-contract sections are explicit none" {
  run bash -c '
    set -euo pipefail
    preview_body="$(awk '"'"'
      /^### 3\.2 Preview$/ { in_section=1; next }
      in_section && /^### / { in_section=0 }
      in_section { print }
    '"'"' docs/project-spec.md)"
    printf "%s\n" "$preview_body" | grep -qx "None."
    ! printf "%s\n" "$preview_body" | grep -q "^- "

    planned_body="$(awk '"'"'
      /^### 3\.3 Planned$/ { in_section=1; next }
      in_section && /^### / { in_section=0 }
      in_section { print }
    '"'"' docs/project-spec.md)"
    printf "%s\n" "$planned_body" | grep -qx "None."
    ! printf "%s\n" "$planned_body" | grep -q "^- "

    non_contract_body="$(awk '"'"'
      /^### 3\.4 Implemented but Non-Contract$/ { in_section=1; next }
      in_section && /^### / { in_section=0 }
      in_section { print }
    '"'"' docs/project-spec.md)"
    printf "%s\n" "$non_contract_body" | grep -qx "None."
    ! printf "%s\n" "$non_contract_body" | grep -q "^- "
  '
  [ "$status" -eq 0 ]
}
