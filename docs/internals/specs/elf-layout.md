# ELF Output Format

**English** | **[中文](elf-layout_zh.md)**

---

### Overview

alinker outputs standard ELF64 executables (x86_64 Linux).

### Layout

```
┌─────────────────────┐ 0x0
│ ELF Header          │
├─────────────────────┤ 0x40
│ Program Header      │
│ (PT_LOAD × 2)       │
├─────────────────────┤
│ .text section (RX)  │ 0x400000
│   - code            │
├─────────────────────┤
│ .data/.bss (RW)     │
│   - globals         │
├─────────────────────┤
│ Section Headers     │ (optional)
└─────────────────────┘
```

### Section Configuration

Typical `link.json` uses two segments:

```json
{
  "segments": [
    {
      "name": ".text",
      "flags": "RX",
      "vaddr": 4194304
    },
    {
      "name": ".data",
      "flags": "RW",
      "vaddr": 4202496
    }
  ]
}
```

| Section | Flags | Align | Description |
|---------|-------|-------|-------------|
| .text | RX | 0x1000 | Code, read-only |
| .data | RW | 0x1000 | Initialized data |
| .bss | RW | - | Uninitialized data (no file space) |

### Entry Point

Default entry symbol: `_start` or first exported function.

Specify in `link.json`:
```json
{
  "entry": "_start"
}
```

### Runtime

Link-time includes:
- `krt.kasm.oobj` — Runtime startup code
- `olrt.kasm.oobj` — OLang runtime
- `libposix.kasm.oobj` — POSIX syscall wrappers

Runtime startup flow:
1. Set up stack
2. Call `main`
3. Call `exit` with `main` return value

---

[Return](../README.md)
