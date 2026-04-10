# Linker

**English** | **[中文](linker_zh.md)**

---

Location: `alinker/`

### Structure

```
alinker/
└── src/
    ├── main.c          # Entry point
    └── link_core.c/h   # Linking core
```

### Linking Flow

```
Input: .oobj files + link.json
     ↓
1. Load all object files
     ↓
2. Merge sections, resolve symbols
     ↓
3. Process relocations
     ↓
4. Execute link script ops
     ↓
Output: ELF or raw binary
```

### Link Script

`link.json` format:

```json
{
  "format": "elf",
  "entry": "_start",
  "segments": [
    { "name": ".text", "flags": "RX", "vaddr": 4194304 }
  ],
  "ops": [
    { "op": "write_blob", "section": ".text", "data": "..." },
    { "op": "write_u64_le", "value": "..." }
  ]
}
```

### Key Functions

```c
// Main linking function
int link_perform(LinkContext *ctx);

// Process relocations
int link_apply_relocs(LinkContext *ctx, OlObjFile *obj);
```

### Supported Ops

| Op | Description |
|----|-------------|
| `write_blob` | Write byte blob |
| `write_u32_le` | Write 32-bit little-endian |
| `write_u64_le` | Write 64-bit little-endian |
| `write_payload` | Write section content |

### Relocation Types

| Type | Description |
|------|-------------|
| `R_X86_64_64` | Absolute 64-bit |
| `R_X86_64_PC32` | PC-relative 32-bit |
| `R_X86_64_32` | Absolute 32-bit |

---

[Return](README.md)
