#!/usr/bin/env bash
# PC64 relocation: 8-byte slot at start of .text gets S - P for symbol at +8 (expect 8).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
BIN="$ROOT/bin/alinker"
FIX="$ROOT/tests/fixtures/alinker_pc64"
OUT="$ROOT/tests/fixtures/alinker_pc64_out.bin"
rm -f "$OUT"
"$BIN" --script "$FIX/link.json" -o "$OUT" -- "$FIX/main.oobj"
python3 - "$OUT" <<'PY'
import struct, sys
path = sys.argv[1]
with open(path, "rb") as f:
    f.seek(0x1000)
    q = f.read(8)
v = struct.unpack("<Q", q)[0]
if v != 8:
    print("alinker_pc64: expected quadword 8, got", v, file=sys.stderr)
    sys.exit(1)
PY
rm -f "$OUT"
echo "OK: alinker_pc64"
