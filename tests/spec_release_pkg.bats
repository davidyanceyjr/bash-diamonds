#!/usr/bin/env bats
# tests/spec_release_pkg.bats

@test "release packaging: make dist emits tarball and checksum with required contents" {
  run bash -c '
    set -euo pipefail
    make dist-clean >/dev/null
    make dist >/dev/null
    tarball="$(ls -1t build/release/diamonds-*.tar.gz | head -n1)"
    [ -n "$tarball" ]
    [ -f "$tarball" ]
    [ -f "$tarball.sha256" ]
    sha256sum -c "$tarball.sha256" >/dev/null
    tar -tzf "$tarball" | grep -E "/builtins/lines\\.so$" >/dev/null
    tar -tzf "$tarball" | grep -E "/install-user\\.sh$" >/dev/null
    tar -tzf "$tarball" | grep -E "/docs/project-spec\\.md$" >/dev/null
    tar -tzf "$tarball" | grep -E "/README\\.md$" >/dev/null
  '
  [ "$status" -eq 0 ]
}

@test "release packaging: make dist is repeatable and refreshes artifact" {
  run bash -c '
    set -euo pipefail
    make dist-clean >/dev/null
    make dist >/dev/null
    tarball="$(ls -1t build/release/diamonds-*.tar.gz | head -n1)"
    before="$(stat -c %Y "$tarball")"
    sleep 1
    make dist >/dev/null
    after="$(stat -c %Y "$tarball")"
    [ "$after" -gt "$before" ]
  '
  [ "$status" -eq 0 ]
}

@test "release packaging: bundled installer runs with --help" {
  run bash -c '
    set -euo pipefail
    make dist-clean >/dev/null
    make dist >/dev/null
    tarball="$(ls -1t build/release/diamonds-*.tar.gz | head -n1)"
    tmpdir="$(mktemp -d)"
    trap "rm -rf \"$tmpdir\"" EXIT
    tar -C "$tmpdir" -xzf "$tarball"
    pkg_dir="$(find "$tmpdir" -mindepth 1 -maxdepth 1 -type d | head -n1)"
    [ -n "$pkg_dir" ]
    [ -x "$pkg_dir/install-user.sh" ]
    "$pkg_dir/install-user.sh" --help >/dev/null
  '
  [ "$status" -eq 0 ]
}

@test "release packaging: checksum validation fails when tarball is modified" {
  run bash -c '
    set -euo pipefail
    make dist-clean >/dev/null
    make dist >/dev/null
    tarball="$(ls -1t build/release/diamonds-*.tar.gz | head -n1)"
    printf "x" >> "$tarball"
    ! sha256sum -c "$tarball.sha256" >/dev/null 2>&1
  '
  [ "$status" -eq 0 ]
}
