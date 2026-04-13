#!/usr/bin/env python3
"""alloc* keywords -> stack/data/bss/rodata/section (placement, not malloc)."""
import re
import sys


def rewrite(s: str) -> str:
    s = re.sub(r"\balloc_section\b", "section", s)
    s = re.sub(r"\balloc_rodata\b", "rodata", s)
    s = re.sub(r"\balloc_data\b", "data", s)
    s = re.sub(r"\balloc_bss\b", "bss", s)
    s = re.sub(r"\balloc\s*\(", "stack(", s)
    return s


def main() -> None:
    for path in sys.argv[1:]:
        with open(path, "r", encoding="utf-8") as f:
            o = f.read()
        n = rewrite(o)
        if n != o:
            with open(path, "w", encoding="utf-8") as f:
                f.write(n)
            print("updated", path)


if __name__ == "__main__":
    main()
