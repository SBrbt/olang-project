#!/usr/bin/env bash
# Every tests/olang/olang_fail/fail_*.ol must be rejected by olang (non-zero exit).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
export OLANG_TOOLCHAIN_ROOT="$ROOT"
OLANG="$ROOT/bin/olang"
FAIL_DIR="$ROOT/tests/olang/olang_fail"
TMP="$ROOT/tests/fixtures/fail_tmp.oobj"
mkdir -p "$(dirname "$TMP")"

shopt -s nullglob
files=("$FAIL_DIR"/fail_*.ol)
shopt -u nullglob

if [[ ${#files[@]} -eq 0 ]]; then
  echo "check_olang_bounds: no $FAIL_DIR/fail_*.ol" >&2
  exit 1
fi

n=0
for f in "${files[@]}"; do
  set +e
  "$OLANG" --target x86_64 --in "$f" -o "$TMP"
  code=$?
  set -e
  if [[ "$code" -eq 0 ]]; then
    echo "check_olang_bounds: expected olang failure for $f" >&2
    exit 1
  fi
  echo "OK: $f (expected compile failure)"
  n=$((n + 1))
done

echo "OK: check_olang_bounds.sh ($n cases)"
