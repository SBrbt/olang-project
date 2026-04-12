#!/usr/bin/env python3
"""One-off migration: old let / @attr let -> let name @stack<T>() / let g @data<T>()."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def transform_line(line: str) -> str:
    s = line.rstrip("\n")
    # @section("x") let name: T = e;  (rare)
    m = re.match(
        r'^(\s*)@section\s*\(\s*"([^"]+)"\s*\)\s+let\s+(\w+)\s*:\s*(.+?)\s*=\s*(.+);\s*$',
        s,
    )
    if m:
        ind, sec, name, ty, expr = m.groups()
        return f'{ind}let {name} @section("{sec}")<{ty}>({expr});'

    # @data|bss|rodata let name: T = expr;
    m = re.match(
        r"^@(\w+)\s+let\s+(\w+)\s*:\s*(.+?)\s*=\s*(.+);\s*$", s
    )
    if m:
        attr, name, ty, expr = m.groups()
        if attr in ("data", "bss", "rodata"):
            return f"let {name} @{attr}<{ty}>({expr});"
        return s

    # @bss let name: T;  (no init)
    m = re.match(r"^@(\w+)\s+let\s+(\w+)\s*:\s*([^;]+);\s*$", s)
    if m:
        attr, name, ty = m.groups()
        if attr in ("data", "bss", "rodata"):
            return f"let {name} @{attr}<{ty}>();"
        return s

    # let name: T = expr;  (local)
    m = re.match(r"^(\s*)let\s+(\w+)\s*:\s*(.+?)\s*=\s*(.+);\s*$", s)
    if m:
        ind, name, ty, expr = m.groups()
        return f"{ind}let {name} @stack<{ty}>({expr});"

    # let name: T;  (no =, aggregate)
    m = re.match(r"^(\s*)let\s+(\w+)\s*:\s*([^;]+);\s*$", s)
    if m:
        ind, name, ty = m.groups()
        return f"{ind}let {name} @stack<{ty}>();"

    return line


def process_file(path: Path) -> bool:
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines(keepends=True)
    out = []
    changed = False
    for line in lines:
        if line.lstrip().startswith("let ") or line.lstrip().startswith("@") and " let " in line:
            new_line = transform_line(line.rstrip("\n"))
            if new_line != line.rstrip("\n"):
                changed = True
                out.append(new_line + ("\n" if line.endswith("\n") else ""))
                continue
        out.append(line)
    if changed:
        path.write_text("".join(out), encoding="utf-8")
    return changed


def main():
    # Skip examples/linux_x86_64/out (build artifacts)
    globs = [
        "examples/linux_x86_64/programs/*.ol",
        "examples/linux_x86_64/include/*.ol",
        "tests/**/*.ol",
    ]
    n = 0
    for pattern in globs:
        for path in sorted(ROOT.glob(pattern)):
            if process_file(path):
                print("updated", path.relative_to(ROOT))
                n += 1
    print("done,", n, "files changed")


if __name__ == "__main__":
    main()
