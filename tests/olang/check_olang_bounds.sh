#!/usr/bin/env bash
# Expect olang to reject fail_long_ident.ol (identifier length).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
export OLANG_TOOLCHAIN_ROOT="$ROOT"
OLANG="$ROOT/bin/olang"
TMP="$ROOT/tests/fixtures/fail_tmp.oobj"
mkdir -p "$(dirname "$TMP")"
set +e
"$OLANG" --in "$ROOT/tests/olang/olang_fail/fail_long_ident.ol" -o "$TMP" 2>/dev/null
code=$?
set -e
if [[ "$code" -eq 0 ]]; then
  echo "check_olang_bounds: expected olang failure on long identifier" >&2
  exit 1
fi
echo "OK: tests/olang/olang_fail/fail_long_ident.ol (expected compile failure)"

TMP2="$ROOT/tests/fixtures/fail_tmp2.oobj"
set +e
"$OLANG" --in "$ROOT/tests/olang/olang_fail/fail_unterminated_string.ol" -o "$TMP2" 2>/dev/null
code2=$?
set -e
if [[ "$code2" -eq 0 ]]; then
  echo "check_olang_bounds: expected olang failure on unterminated string" >&2
  exit 1
fi
echo "OK: tests/olang/olang_fail/fail_unterminated_string.ol (expected compile failure)"

echo "OK: check_olang_bounds.sh (2 cases)"
