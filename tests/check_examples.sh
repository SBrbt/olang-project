#!/usr/bin/env bash
# Build toolchain and run tests in examples directory.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
export OLANG_TOOLCHAIN_ROOT="$ROOT"
make clean >/dev/null 2>&1 || true
make all
bash "$ROOT/tests/olang/run_programs_olc.sh"
bash "$ROOT/tests/olang/check_olang_bounds.sh"
echo "OK: examples check"
