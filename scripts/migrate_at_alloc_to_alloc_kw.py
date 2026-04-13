#!/usr/bin/env python3
"""Migrate @stack|@data|... (w,...)(init) to alloc*(w,...,init)."""

import sys


def skip_paren_group(s: str, open_idx: int) -> int:
    """open_idx points at '('. Returns index after matching ')'."""
    assert s[open_idx] == "("
    depth = 1
    i = open_idx + 1
    n = len(s)
    while i < n and depth:
        c = s[i]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif c == '"':
            i += 1
            while i < n:
                if s[i] == "\\":
                    i += 2
                    continue
                if s[i] == '"':
                    break
                i += 1
        i += 1
    return i


def merge_two_groups(prefix: str, kw: str, s: str) -> str:
    """Replace kw + '(' group1 ')' '(' group2 ')' with prefix + '(' merged ')' ."""
    needle = kw
    out = []
    i = 0
    n = len(s)
    while i < n:
        j = s.find(needle, i)
        if j < 0:
            out.append(s[i:])
            break
        out.append(s[i:j])
        k = j + len(needle)
        while k < n and s[k] in " \t\r\n":
            k += 1
        if k >= n or s[k] != "(":
            out.append(s[j:k])
            i = k
            continue
        g1_end = skip_paren_group(s, k)
        g2_start = g1_end
        while g2_start < n and s[g2_start] in " \t\r\n":
            g2_start += 1
        if g2_start >= n or s[g2_start] != "(":
            out.append(s[j:g1_end])
            i = g1_end
            continue
        g2_end = skip_paren_group(s, g2_start)
        inner1 = s[k + 1 : g1_end - 1]
        inner2 = s[g2_start + 1 : g2_end - 1]
        merged = inner1.strip()
        if inner2.strip():
            merged = merged + ", " + inner2.strip()
        out.append(prefix + "(" + merged + ")")
        i = g2_end
    return "".join(out)


def migrate_section(s: str) -> str:
    needle = "@section"
    out = []
    i = 0
    n = len(s)
    while i < n:
        j = s.find(needle, i)
        if j < 0:
            out.append(s[i:])
            break
        out.append(s[i:j])
        k = j + len(needle)
        while k < n and s[k] in " \t\r\n":
            k += 1
        if k >= n or s[k] != "(":
            out.append(s[j:k])
            i = k
            continue
        g0_end = skip_paren_group(s, k)  # ("name")
        g1_start = g0_end
        while g1_start < n and s[g1_start] in " \t\r\n":
            g1_start += 1
        if g1_start >= n or s[g1_start] != "(":
            out.append(s[j:g0_end])
            i = g0_end
            continue
        g1_end = skip_paren_group(s, g1_start)
        g2_start = g1_end
        while g2_start < n and s[g2_start] in " \t\r\n":
            g2_start += 1
        if g2_start >= n or s[g2_start] != "(":
            out.append(s[j:g1_end])
            i = g1_end
            continue
        g2_end = skip_paren_group(s, g2_start)
        sec = s[k + 1 : g0_end - 1].strip()
        inner1 = s[g1_start + 1 : g1_end - 1].strip()
        inner2 = s[g2_start + 1 : g2_end - 1].strip()
        merged = sec + ", " + inner1
        if inner2:
            merged = merged + ", " + inner2
        out.append("alloc_section(" + merged + ")")
        i = g2_end
    return "".join(out)


def migrate_file(path: str) -> None:
    with open(path, "r", encoding="utf-8") as f:
        s = f.read()
    orig = s
    s = migrate_section(s)
    s = merge_two_groups("alloc", "@stack", s)
    s = merge_two_groups("alloc_data", "@data", s)
    s = merge_two_groups("alloc_bss", "@bss", s)
    s = merge_two_groups("alloc_rodata", "@rodata", s)
    if s != orig:
        with open(path, "w", encoding="utf-8") as f:
            f.write(s)
        print("updated", path)


def main() -> None:
    for path in sys.argv[1:]:
        migrate_file(path)


if __name__ == "__main__":
    main()
