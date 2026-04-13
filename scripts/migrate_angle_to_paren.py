#!/usr/bin/env python3
"""Migrate OLang angle syntax to parentheses (see parser.c)."""
import re
import sys


def take_balanced_angle(s: str, start_lt: int) -> tuple[int, str]:
    """start_lt points at '<'. Returns (index_after_gt, inner_without_brackets)."""
    assert s[start_lt] == "<"
    j = start_lt + 1
    depth = 1
    n = len(s)
    while j < n and depth > 0:
        c = s[j]
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        j += 1
    if depth != 0:
        raise ValueError("unbalanced < > at %d" % start_lt)
    return j, s[start_lt + 1 : j - 1]


def replace_prefix_angle(s: str, prefix: str, repl_name: str) -> str:
    """prefix must be 'name<' e.g. sizeof< ; angle bracket starts at len(name)."""
    assert prefix.endswith("<")
    name = prefix[:-1]
    out = []
    i = 0
    n = len(s)
    nlen = len(name)
    while i < n:
        if s.startswith(prefix, i):
            lt = i + nlen
            end, inner = take_balanced_angle(s, lt)
            out.append(repl_name + "(" + inner + ")")
            i = end
        else:
            out.append(s[i])
            i += 1
    return "".join(out)


def migrate_at_alloc(s: str) -> str:
    out = []
    i = 0
    n = len(s)
    while i < n:
        if s[i] == "@" and i + 1 < n and (s[i + 1].isalpha() or s[i + 1] == "_"):
            k = i + 1
            while k < n and (s[k].isalnum() or s[k] == "_"):
                k += 1
            name = s[i + 1 : k]
            if k < n and s[k] == "<":
                end, inner = take_balanced_angle(s, k)
                m = end
                while m < n and s[m] in " \t\r\n":
                    m += 1
                if m < n and s[m] == "(":
                    out.append("@" + name + "(" + inner + ")")
                    i = end
                    continue
            out.append(s[i])
            i += 1
            continue
        out.append(s[i])
        i += 1
    return "".join(out)


CAST_TYPES = (
    "bool",
    "i8",
    "i16",
    "i32",
    "i64",
    "u8",
    "u16",
    "u32",
    "u64",
    "f16",
    "f32",
    "f64",
    "b8",
    "b16",
    "b32",
    "b64",
    "ptr",
)


def migrate_section_alloc(s: str) -> str:
    return re.sub(r'(@section\("[^"]*"\))\s*<([^>]+)>\s*\(', r"\1(\2)(", s)


def migrate_let_ref_bind(s: str) -> str:
    """` <T>addr` / ` <T>(find` -> ` (T)addr` / ` (T)(find`."""
    s = re.sub(r" <([A-Za-z0-9_]+)>\s*addr\b", r" (\1)addr", s)
    s = re.sub(r" <([A-Za-z0-9_]+)>\s*\(", r" (\1)(", s)
    return s


def migrate(text: str) -> str:
    t = text
    t = replace_prefix_angle(t, "sizeof<", "sizeof")
    t = migrate_section_alloc(t)
    t = migrate_at_alloc(t)
    for kw in ("load", "find", "store"):
        t = replace_prefix_angle(t, kw + "<", kw)
    for ty in CAST_TYPES:
        t = replace_prefix_angle(t, ty + "<", ty)
    t = re.sub(r"(\btype\s+\w+\s*=\s*)array<", r"\1array(", t)
    t = re.sub(r"array\(([^,()]+),\s*(\d+)\)>", r"array(\1, \2)", t)
    t = re.sub(r"array\(([^,]+),\s*(\d+)>", r"array(\1, \2)", t)
    t = migrate_let_ref_bind(t)
    return t


def main() -> None:
    paths = sys.argv[1:]
    if not paths:
        print("usage: migrate_angle_to_paren.py <file.ol> ...", file=sys.stderr)
        sys.exit(2)
    for p in paths:
        with open(p, encoding="utf-8") as f:
            old = f.read()
        new = migrate(old)
        if new != old:
            with open(p, "w", encoding="utf-8") as f:
                f.write(new)
            print("updated", p)


if __name__ == "__main__":
    main()
