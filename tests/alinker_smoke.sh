#!/usr/bin/env bash
# Minimal end-to-end: alinker produces non-empty output from fixture .oobj + script.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
BIN="$ROOT/bin/alinker"
FIX="$ROOT/tests/fixtures/alinker_smoke"
OUT="$ROOT/tests/fixtures/alinker_smoke_out.bin"
rm -f "$OUT"
"$BIN" -v --script "$FIX/link.json" -o "$OUT" -- "$FIX/main.oobj"
if ! test -s "$OUT"; then
  echo "alinker_smoke: output missing or empty: $OUT" >&2
  exit 1
fi
# Magic bytes at payload start (file_off 0x1000): first byte of merged image is .text
# With prepend_call_stub false, payload is raw section bytes — expect 0xC3 (ret)
python3 - "$OUT" <<'PY'
import sys
path = sys.argv[1]
with open(path, "rb") as f:
    f.seek(0x1000)
    b = f.read(1)
if b != b"\xc3":
    print("alinker_smoke: expected ret opcode at payload start, got", repr(b), file=sys.stderr)
    sys.exit(1)
PY
rm -f "$OUT"
echo "OK: alinker_smoke"
