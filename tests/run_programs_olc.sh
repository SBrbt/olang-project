#!/usr/bin/env bash
# Compile programs/ex_*.ol with examples/olc and run checks; test two .ol unit linking at the end.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TOOLCHAIN_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
EX="$TOOLCHAIN_ROOT/examples"
PROG="$EX/programs"
OUT="$EX/out"
OLC="$TOOLCHAIN_ROOT/examples/olc"

mkdir -p "$OUT"

shopt -s nullglob
EXAMPLES=()
for f in "$PROG"/ex_*.ol; do
  EXAMPLES+=("$(basename "$f" .ol)")
done
shopt -u nullglob

if [[ ${#EXAMPLES[@]} -eq 0 ]]; then
  echo "run_programs_olc: no $PROG/ex_*.ol" >&2
  exit 1
fi

for name in "${EXAMPLES[@]}"; do
  "$OLC" -o "$OUT/${name}.elf" "$PROG/${name}.ol"
  chmod +x "$OUT/${name}.elf"
done

check_exit() {
  local name="$1"
  local want="$2"
  set +e
  "$OUT/${name}.elf"
  local code=$?
  set -e
  if [[ "$code" -ne "$want" ]]; then
    echo "run_programs_olc: ${name}.elf exit ${code}, expected ${want}" >&2
    exit 1
  fi
}

for name in "${EXAMPLES[@]}"; do
  if [[ "$name" != "ex_hello" && "$name" != "ex_write_ok" && "$name" != "ex_stdout_two" ]]; then
    check_exit "$name" 0
  fi
done

set +e
"$OUT/ex_hello.elf" >"$OUT/ex_hello.stdout"
code_h=$?
set -e
if [[ "$code_h" -ne 0 ]]; then
  echo "run_programs_olc: ex_hello exit $code_h" >&2
  exit 1
fi
if ! grep -q "Hello from OLang" "$OUT/ex_hello.stdout"; then
  echo "run_programs_olc: ex_hello missing expected stdout" >&2
  cat "$OUT/ex_hello.stdout" >&2 || true
  exit 1
fi

set +e
"$OUT/ex_write_ok.elf" >"$OUT/ex_write_ok.stdout"
code_w=$?
set -e
if [[ "$code_w" -ne 0 ]]; then
  echo "run_programs_olc: ex_write_ok exit $code_w" >&2
  exit 1
fi
if ! grep -q "OK" "$OUT/ex_write_ok.stdout"; then
  echo "run_programs_olc: ex_write_ok missing OK" >&2
  cat "$OUT/ex_write_ok.stdout" >&2 || true
  exit 1
fi

set +e
"$OUT/ex_stdout_two.elf" >"$OUT/ex_stdout_two.stdout"
code_s2=$?
set -e
if [[ "$code_s2" -ne 0 ]]; then
  echo "run_programs_olc: ex_stdout_two exit $code_s2" >&2
  exit 1
fi
if ! grep -q "AB" "$OUT/ex_stdout_two.stdout"; then
  echo "run_programs_olc: ex_stdout_two missing expected stdout" >&2
  cat "$OUT/ex_stdout_two.stdout" >&2 || true
  exit 1
fi

echo "OK: run_programs_olc.sh (${#EXAMPLES[@]} programs)"

# Two translation unit linking (filenames not starting with ex_ are excluded from the single-file batch above)
"$OLC" -o "$OUT/multi_file.elf" "$PROG/multi_file_main.ol" "$PROG/multi_file_lib.ol"
chmod +x "$OUT/multi_file.elf"
set +e
"$OUT/multi_file.elf"
code_mf=$?
set -e
if [[ "$code_mf" -ne 42 ]]; then
  echo "run_programs_olc: multi-file link expected exit 42, got $code_mf" >&2
  exit 1
fi
echo "OK: multi_file link"
