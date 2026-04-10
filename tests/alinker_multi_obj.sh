#!/usr/bin/env bash
# Two .oobj merged: ABS64 cross-file relocation for 'helper'.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/bin/alinker"
FIX="$ROOT/tests/fixtures/alinker_multi_obj"
OUT="$ROOT/tests/fixtures/alinker_multi_obj_out.bin"
rm -f "$OUT"
"$BIN" --script "$FIX/link.json" -o "$OUT" -- "$FIX/a.oobj" "$FIX/b.oobj"
python3 - "$OUT" <<'PY'
import struct, sys
path = sys.argv[1]
with open(path, "rb") as f:
    f.seek(0x1000)
    q = f.read(8)
v = struct.unpack("<Q", q)[0]
expect = 0x401000 + 8
if v != expect:
    print("alinker_multi_obj: expected abs64", hex(expect), "got", hex(v), file=sys.stderr)
    sys.exit(1)
PY
rm -f "$OUT"
echo "OK: alinker_multi_obj"
