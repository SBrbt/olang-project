#!/usr/bin/env bash
# .bytes with tab separator must work.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/bin/kasm"
TMP="$ROOT/tests/fixtures/kasm_bytes_tab_tmp"
mkdir -p "$TMP"

printf '.global _start\n_start:\n.bytes\tcc 90\n' > "$TMP/test.kasm"

"$BIN" --isa "$ROOT/kasm/isa/x86_64_linux.json" --in "$TMP/test.kasm" -o "$TMP/out.oobj"
rm -rf "$TMP"
echo "OK: kasm_bytes_tab"
