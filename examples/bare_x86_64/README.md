# Bare-metal x86_64 (link script only)

**English** | **[中文](README_zh.md)**

---

This directory is **not** Linux-specific: it holds an [`alinker`](../../alinker/) script for a **flat raw binary** (no ELF, no OS assumptions).

| Path | Purpose |
|------|---------|
| [`link/raw.bin.json`](link/raw.bin.json) | Example: single load segment, `write_payload` at file offset `0`, `prepend_call_stub: false`. Adjust `vaddr` / sections for your bootloader or emulator. |

See also: [docs/internals/specs/elf-layout.md](../../docs/internals/specs/elf-layout.md).
