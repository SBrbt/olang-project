#!/usr/bin/env bash
# Test olprep #include "..." expansion.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
FIX="$ROOT/tests/fixtures/preproc_include"
BIN="$ROOT/bin/olprep"
OUT="$FIX/got.ol"
EXP="$FIX/expected.ol"

if [[ ! -x "$BIN" ]]; then
  echo "preproc_include: missing $BIN (make olprep)" >&2
  exit 1
fi

rm -f "$OUT"
"$BIN" --in "$FIX/includer.ol" -o "$OUT"

if ! diff -u "$EXP" "$OUT"; then
  echo "preproc_include: output mismatch" >&2
  exit 1
fi

rm -f "$OUT"
echo "OK: preproc_include.sh"
