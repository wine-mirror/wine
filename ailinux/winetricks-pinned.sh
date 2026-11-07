#!/bin/sh
set -eu
ROOT="$HOME/workspace/.wine-build-state/winetricks-upstream"
test "$(git -C "$ROOT" describe --tags --exact-match 2>/dev/null)" = 20260125
SHA=$(sha256sum "$ROOT/src/winetricks" | cut -d' ' -f1)
test "$SHA" = 431f82fc74000e6c864409f1d8fb495d696c03928808e3e8acffc45179312a7b
exec "$ROOT/src/winetricks" "$@"
