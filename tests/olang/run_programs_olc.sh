#!/usr/bin/env bash
# Compile and run each listed program with examples/linux_x86_64/olc; one OK line per test.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TOOLCHAIN_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
LX="$TOOLCHAIN_ROOT/examples/linux_x86_64"
PROG="$LX/programs"
OUT="$LX/out"
OLC="$LX/olc"

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

mapfile -t EXAMPLES < <(printf '%s\n' "${EXAMPLES[@]}" | sort)

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
    echo "OK: examples/linux_x86_64/programs/${name}.ol (exit 0)"
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
echo "OK: examples/linux_x86_64/programs/ex_hello.ol (stdout)"

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
echo "OK: examples/linux_x86_64/programs/ex_write_ok.ol (stdout)"

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
echo "OK: examples/linux_x86_64/programs/ex_stdout_two.ol (stdout)"

# Two translation units
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
echo "OK: examples/linux_x86_64/programs/multi_file_main.ol + multi_file_lib.ol (exit 42)"

echo "OK: run_programs_olc.sh (${#EXAMPLES[@]} ex_*.ol + multi_file)"
