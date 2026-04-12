#!/usr/bin/env bash
# Verify Markdown [text](path/to/file.ol) targets exist (paths relative to each .md file).
# README*.md at repo root and docs/**/*.md
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

fail=0
check_md() {
  local md="$1"
  local base rel raw target
  base=$(dirname "$md")
  rel="${md#"$ROOT"/}"
  while IFS= read -r raw; do
    [[ -z "$raw" ]] && continue
    case "$raw" in
      http://*|https://*) continue ;;
    esac
    target=$(cd "$base" && realpath -m "$raw" 2>/dev/null) || continue
    case "$target" in
      "$ROOT"/*) ;;
      *) continue ;;
    esac
    if [[ ! -f "$target" ]]; then
      echo "verify_doc_olang_refs: broken .ol link: $rel: '$raw' -> $target" >&2
      fail=1
    fi
  done < <(grep -oP '\[[^\]]*\]\(\s*\K[^)#\s]+\.ol(?=\s*\))' "$md" 2>/dev/null || true)
}

for f in README.md README_zh.md; do
  [[ -f "$f" ]] && check_md "$(realpath "$f")"
done
if [[ -d docs ]]; then
  while IFS= read -r -d '' md; do
    check_md "$(realpath "$md")"
  done < <(find docs -name '*.md' -print0 2>/dev/null || true)
fi

if [[ "$fail" -ne 0 ]]; then
  exit 1
fi
echo "OK: all Markdown .ol link targets resolve to files"
