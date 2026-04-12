#!/usr/bin/env python3
"""One-off migration: let x @stack<T>(...) -> let x<T> @stack<bits>(...), same for @data/@bss/@rodata/@section."""

from __future__ import annotations

import re
import sys
from pathlib import Path

BUILTIN_BITS = {
    "bool": 8,
    "u8": 8,
    "i8": 8,
    "b8": 8,
    "u16": 16,
    "i16": 16,
    "f16": 16,
    "b16": 16,
    "u32": 32,
    "i32": 32,
    "f32": 32,
    "b32": 32,
    "u64": 64,
    "i64": 64,
    "f64": 64,
    "b64": 64,
    "ptr": 64,
}

STRUCT_RE = re.compile(
    r"type\s+(\w+)\s*=\s*struct\s*\{([^}]*)\}\s*;",
    re.MULTILINE | re.DOTALL,
)
ARRAY_RE = re.compile(
    r"type\s+(\w+)\s*=\s*array\s*<\s*(\w+)\s*,\s*(\d+)\s*>\s*;",
)


def field_bytes(body: str, sizes: dict[str, int]) -> int | None:
    total = 0
    for raw in re.split(r"[,;]", body):
        line = raw.strip()
        if not line or line.startswith("//"):
            continue
        m = re.search(r":\s*(\w+)", line)
        if not m:
            continue
        t = m.group(1)
        if t not in sizes:
            return None
        total += sizes[t]
    return total


def compute_typedef_sizes(text: str) -> dict[str, int]:
    sizes: dict[str, int] = {k: v // 8 for k, v in BUILTIN_BITS.items()}

    for m in ARRAY_RE.finditer(text):
        name, elem, count_s = m.group(1), m.group(2), m.group(3)
        n = int(count_s)
        if elem not in sizes:
            continue
        sizes[name] = sizes[elem] * n

    changed = True
    while changed:
        changed = False
        for m in STRUCT_RE.finditer(text):
            name, body = m.group(1), m.group(2)
            fb = field_bytes(body, sizes)
            if fb is not None and fb > 0 and sizes.get(name) != fb:
                sizes[name] = fb
                changed = True
    return sizes


# let name @stack<ty>(...) ;  — also data, bss, rodata, section("sec")
LET_RE = re.compile(
    r"\blet\s+(\w+)\s+@(stack|data|bss|rodata|section\([^)]*\))\s*<([^>]+)>\s*(\([^;]*\))\s*;",
)


def migrate_content(text: str) -> str:
    sizes = compute_typedef_sizes(text)

    out: list[str] = []
    last = 0
    for m in LET_RE.finditer(text):
        out.append(text[last : m.start()])
        name, alloc_kw, ty, paren = m.group(1), m.group(2), m.group(3), m.group(4)
        if ty not in sizes:
            raise ValueError(f"unknown type size for {ty!r} in let {name} ...")
        bits = sizes[ty] * 8
        out.append(f"let {name}<{ty}> @{alloc_kw}<{bits}>{paren};")
        last = m.end()
    out.append(text[last:])
    return "".join(out)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    paths = list((root / "examples" / "programs").rglob("*.ol"))
    paths += list((root / "tests").rglob("*.ol"))
    for path in sorted(paths):
        text = path.read_text(encoding="utf-8")
        if " @stack<" not in text and " @data<" not in text and " @bss<" not in text and " @rodata<" not in text:
            if "@section" in text and "let " in text:
                pass
            else:
                continue
        try:
            new = migrate_content(text)
        except ValueError as e:
            print(f"FAIL {path}: {e}", file=sys.stderr)
            sys.exit(1)
        if new != text:
            path.write_text(new, encoding="utf-8")
            print(path)


if __name__ == "__main__":
    main()
