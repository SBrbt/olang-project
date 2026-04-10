# Object File Format (.oobj)

**English** | **[中文](oobj_zh.md)**

---

### Overview

.oobj is the intermediate object format for the OLang toolchain, used by olang/kasm output and alinker input.

### File Structure

```
┌──────────────────┐
│  Header          │
├──────────────────┤
│  Section Table   │
├──────────────────┤
│  Symbol Table    │
├──────────────────┤
│  Reloc Table     │
├──────────────────┤
│  Section Data    │
└──────────────────┘
```

### C Structure Definitions

```c
typedef struct OlObjHeader {
    char magic[4];        // "OOBJ"
    uint32_t version;
    uint32_t num_sections;
    uint32_t num_symbols;
    uint32_t num_relocs;
} OlObjHeader;

typedef struct OlObjSection {
    char name[32];
    uint32_t flags;       // SHF_*
    uint32_t data_len;
    uint32_t data_offset; // Offset from file start
} OlObjSection;

typedef struct OlObjSymbol {
    char name[64];
    uint64_t value;
    uint32_t section;     // Section index, 0=undefined
    uint32_t flags;       // STB_*, STT_*
} OlObjSymbol;

typedef struct OlObjReloc {
    uint64_t offset;      // Relocation position (section offset)
    uint32_t section;     // Target section
    uint32_t symbol;      // Symbol index
    uint32_t type;        // R_X86_64_*
    int64_t addend;
} OlObjReloc;
```

### Section Flags

| Flag | Value | Description |
|------|-------|-------------|
| `SHF_WRITE` | 0x1 | Writable |
| `SHF_ALLOC` | 0x2 | Load into memory |
| `SHF_EXEC` | 0x4 | Executable |

### Symbol Flags

| Flag | Value | Description |
|------|-------|-------------|
| `STB_LOCAL` | 0x0 | Local symbol |
| `STB_GLOBAL` | 0x1 | Global symbol |
| `STB_WEAK` | 0x2 | Weak symbol |
| `STT_FUNC` | 0x10 | Function |
| `STT_OBJECT` | 0x20 | Data object |

### Relocation Types

| Type | Value | Description |
|------|-------|-------------|
| `R_X86_64_64` | 1 | Absolute 64-bit |
| `R_X86_64_PC32` | 2 | PC-relative 32-bit |
| `R_X86_64_32` | 10 | Absolute 32-bit |

### Tools

`obinutils` provides:
- `oobj-dump` — Display object file contents
- `oobj-nm` — List symbol table
- `oobj-diff` — Compare two object files

---

[Return](../README.md)
