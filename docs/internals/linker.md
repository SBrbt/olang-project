# Linker (alinker)

**English** | **[中文](linker_zh.md)**

---

Location: `alinker/`

### Role

`alinker` merges `.oobj` inputs according to a **link script** (JSON) and writes the output file. The core **does not assume** a particular executable format, OS, or ABI: it only merges sections and symbols, applies relocations, concatenates payload bytes in `load_groups` order, and runs the script’s **`layout`** operations (writes at explicit file offsets). Whatever bytes the script places at the start of the file (ELF header blobs, raw images, etc.) are **script-defined**, not built into the linker as “ELF knowledge.”

Optional **entry stub** bytes (`call_stub_hex`) are copied from the script; if the first byte is `0xE8`, the linker patches the **rel32** at offset +1 so a `call` reaches the symbol named by `"entry"`. No other stub semantics are hardcoded.

### Structure

```
alinker/
└── src/
    ├── main.c          # CLI
    └── link_core.c/h   # Merge, relocate, layout eval, emit
```

### Linking flow

```
Input: .oobj files + link.json
     ↓
1. Load and merge objects (sections, symbols, relocs)
     ↓
2. Apply relocations using computed section bases / VMAs from the script
     ↓
3. Build contiguous payload (optional stub from script + section bytes per load_groups)
     ↓
4. Evaluate layout ops (write_blob / write_u32_le / write_u64_le / write_payload)
     ↓
Output: whatever file the script describes
```

### Link script

The script drives **everything**: `entry`, `segments`, `load_groups`, `prepend_call_stub`, `call_stub_hex`, `layout`, etc. See `common/link_script.c` and scripts under `examples/linux_x86_64/link/` (Linux ELF) and `examples/bare_x86_64/link/` (flat raw image).

- **`"entry"`** — symbol name used to resolve the entry point and to patch a leading `call` stub when `call_stub_hex` is present.
- **`call_stub_hex`** — required when **`prepend_call_stub`** is true: raw bytes prepended before merged section data (no default stub inside alinker).

### API

```c
int alinker_link_oobj_only(const LinkScript *script, const char *output_path, int verbose, char *err, size_t err_len);
```

### Relocation kinds (in `.oobj`)

The merger understands `OOBJ` relocation records (`ABS64`, `PC32`, `PC64`); naming in docs may mirror x86 habits, but the linker is **generic** over “patch N bits at offset.”

---

[Return](README.md)
