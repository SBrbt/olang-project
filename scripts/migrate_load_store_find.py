#!/usr/bin/env python3
"""Migrate load(expr)->load[expr], store(a,b)->store[a,b], find(expr)->find[expr]."""
from __future__ import annotations

import sys
from pathlib import Path


def replace_kw_calls(content: str, kw: str) -> str:
    prefix = kw + "("
    while True:
        idx = content.find(prefix)
        if idx < 0:
            break
        open_paren = idx + len(prefix) - 1
        assert content[open_paren] == "("
        depth = 0
        k = open_paren
        while k < len(content):
            c = content[k]
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    inner = content[open_paren + 1 : k]
                    content = content[:idx] + kw + "[" + inner + "]" + content[k + 1 :]
                    break
            k += 1
        else:
            raise RuntimeError(f"unbalanced paren for {kw} at {idx}")
    return content


def migrate_text(content: str) -> str:
    for _ in range(256):
        old = content
        for kw in ("store", "load", "find"):
            content = replace_kw_calls(content, kw)
        if content == old:
            break
    else:
        raise RuntimeError("migrate did not converge")
    return content


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
