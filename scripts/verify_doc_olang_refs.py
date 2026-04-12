#!/usr/bin/env python3
"""
Verify that Markdown link targets pointing at local .ol files exist.

Resolves paths relative to the file containing the link (same rules as browsers /
static site generators). Only checks [text](path/to/file.ol), not shell examples.
Run from repo root: python3 scripts/verify_doc_olang_refs.py
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# [label](path) — capture path; allow spaces common in formatters
LINK_TO_OL = re.compile(r"\[[^\]]*\]\(\s*([^)#\s]+\.ol)\s*\)")


def iter_markdown_files() -> list[Path]:
    out: list[Path] = []
    for name in ("README.md", "README_zh.md"):
        p = ROOT / name
        if p.is_file():
            out.append(p)
    doc = ROOT / "docs"
    if doc.is_dir():
        out.extend(sorted(doc.rglob("*.md")))
    return out


def main() -> int:
    errors: list[tuple[Path, str, Path]] = []
    for md in iter_markdown_files():
        text = md.read_text(encoding="utf-8")
        base = md.parent
        for m in LINK_TO_OL.finditer(text):
            raw = m.group(1).strip()
            if raw.startswith("http://") or raw.startswith("https://"):
                continue
            target = (base / raw).resolve()
            try:
                target.relative_to(ROOT)
            except ValueError:
                continue
            if not target.is_file():
                errors.append((md.relative_to(ROOT), raw, target))

    if errors:
        print("verify_doc_olang_refs: broken .ol link targets:", file=sys.stderr)
        for md_rel, raw, resolved in errors:
            print(f"  {md_rel}: {raw!r} -> {resolved}", file=sys.stderr)
        return 1
    print("OK: all Markdown .ol link targets resolve to files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
