#!/usr/bin/env bats
# tests/spec_doc_links.bats

@test "docs: internal docs references resolve to existing files and anchors" {
  run bash -c '
    set -euo pipefail

    slugify() {
      printf "%s" "$1" | awk '"'"'
        {
          s = tolower($0)
          gsub(/[^a-z0-9]+/, "-", s)
          gsub(/^-+/, "", s)
          gsub(/-+$/, "", s)
          print s
        }
      '"'"'
    }

    heading_has_anchor() {
      local file="$1"
      local anchor="$2"
      local line slug
      while IFS= read -r line; do
        case "$line" in
          \#*)
            line="${line#\#}"
            while [[ "$line" == \#* ]]; do
              line="${line#\#}"
            done
            line="${line# }"
            slug="$(slugify "$line")"
            if [[ "$slug" == "$anchor" ]]; then
              return 0
            fi
            ;;
        esac
      done < "$file"
      return 1
    }

    while IFS= read -r doc; do
      while IFS= read -r ref; do
        [[ -n "$ref" ]] || continue
        path="${ref%%#*}"
        anchor=""
        if [[ "$ref" == *#* ]]; then
          anchor="${ref#*#}"
        fi

        [[ -f "$path" ]] || {
          echo "missing docs target: $doc -> $ref" >&2
          exit 1
        }

        if [[ -n "$anchor" ]] && ! heading_has_anchor "$path" "$anchor"; then
          echo "missing docs anchor: $doc -> $ref" >&2
          exit 1
        fi
      done < <(grep -Eo "docs/[A-Za-z0-9._/-]+\\.md(#[A-Za-z0-9._-]+)?" "$doc" | sort -u || true)
    done < <(find docs -maxdepth 1 -type f -name "*.md" | sort)
  '
  [ "$status" -eq 0 ]
}

@test "docs: count documents its no-exit-1 exception" {
  run grep -F 'never exits with code `1`' docs/count.md
  [ "$status" -eq 0 ]
}
