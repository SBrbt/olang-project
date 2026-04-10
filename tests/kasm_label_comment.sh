#!/usr/bin/env bash
# Labels with inline comments must assemble correctly.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/bin/kasm"
TMP="$ROOT/tests/fixtures/kasm_label_comment_tmp"
mkdir -p "$TMP"

cat > "$TMP/test.kasm" <<'ASM'
.global _start
_start: # entry point
  inst ret
ASM

"$BIN" --isa "$ROOT/kasm/isa/x86_64_linux.json" --in "$TMP/test.kasm" -o "$TMP/out.oobj"
"$ROOT/bin/obinutils" oobj-nm "$TMP/out.oobj" | grep -q '_start'
rm -rf "$TMP"
echo "OK: kasm_label_comment"
