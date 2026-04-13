#!/usr/bin/env python3
"""One-shot migration: stack(...)->stack[...], data(...), bss(...), rodata(...), section(...)."""
from __future__ import annotations

import re
import sys
from pathlib import Path


def extract_paren_content(s: str, open_idx: int) -> tuple[str, int] | None:
    """s[open_idx] == '('. Return (inner, index_after_closing_paren)."""
    if open_idx >= len(s) or s[open_idx] != "(":
        return None
    depth = 0
    i = open_idx
    while i < len(s):
        c = s[i]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                inner = s[open_idx + 1 : i]
                return inner, i + 1
        i += 1
    return None


def transform_inner_stack_data_rodata(inner: str, kw: str) -> str:
    inner = inner.strip()
    # sizeof(T) or sizeof(T), expr
    if inner.startswith("sizeof"):
        return inner  # stack[sizeof(T)] or stack[sizeof(T), expr]
    # leading integer width: \d+ , rest
    m = re.match(r"^(\d+)\s*,\s*(.+)$", inner, re.DOTALL)
    if m:
        return m.group(2).strip()
    return inner


def transform_inner_bss(inner: str) -> str:
    inner = inner.strip()
    if inner.startswith("sizeof"):
        return inner
    m = re.match(r"^(\d+)\s*$", inner)
    if m:
        return m.group(1)
    return inner


def transform_inner_section(inner: str) -> str:
    """section("name", bitwidth, expr) -> "name", expr (infer width)."""
    inner = inner.strip()
    mm = re.match(r'^("(?:[^"\\]|\\.)*")\s*,\s*(.*)$', inner, re.DOTALL)
    if not mm:
        return inner
    q = mm.group(1)
    rest = mm.group(2).strip()
    rm = re.match(r"^(\d+)\s*,\s*(.+)$", rest, re.DOTALL)
    if rm:
        rest = rm.group(2).strip()
    return f"{q}, {rest}"


def migrate_text(text: str) -> str:
    keywords = ["stack", "data", "rodata", "bss", "section"]
    out = []
    pos = 0
    while pos < len(text):
        matched = False
        for kw in keywords:
            if text.startswith(kw + "(", pos):
                ec = extract_paren_content(text, pos + len(kw))
                if not ec:
                    break
                inner, after = ec
                if kw == "bss":
                    new_inner = transform_inner_bss(inner)
                elif kw == "section":
                    new_inner = transform_inner_section(inner)
                else:
                    new_inner = transform_inner_stack_data_rodata(inner, kw)
                out.append(text[pos : pos + len(kw)])
                out.append("[")
                out.append(new_inner)
                out.append("]")
                pos = after
                matched = True
                break
        if not matched:
            out.append(text[pos])
            pos += 1
    return "".join(out)


def main() -> None:
    paths = [Path(p) for p in sys.argv[1:]] if len(sys.argv) > 1 else []
    if not paths:
        root = Path(__file__).resolve().parents[1]
        paths = list(root.glob("examples/linux_x86_64/programs/*.ol"))
        paths += list(root.glob("tests/**/*.ol"))
        paths += list(root.glob("docs/**/*.md"))
    for p in paths:
        if not p.is_file():
            continue
        s = p.read_text(encoding="utf-8")
        t = migrate_text(s)
        if t != s:
            p.write_text(t, encoding="utf-8")
            print("updated", p)


if __name__ == "__main__":
    main()
