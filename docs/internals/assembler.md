# Assembler (kasm)

**English** | **[中文](assembler_zh.md)**

---

Location: `kasm/`

### Role

`kasm` turns a `.kasm` source file into a `.oobj` object (sections, symbols, relocations). It is **not** tied to a particular OS: which ISA JSON you pass (`--isa`) defines which `inst` mnemonics exist.

### Layout

```
kasm/
├── src/
│   ├── main.c          # CLI (--in, -o, optional --isa)
│   ├── kasm_asm.c/h    # Line-oriented assembly
│   └── kasm_isa.c/h    # Loads ISA JSON into `IsaSpec`
└── isa/
    └── x86_64.json     # Example ISA: fixed-prefix opcodes + optional immediates + pc32 relocs
```

### ISA JSON (`x86_64.json`)

Each instruction object has:

| Field | Meaning |
|-------|---------|
| `mnemonic` | Name used after `inst` |
| `bytes` | Hex string: opcode bytes emitted first |
| `imm_bits` | `0`, `8`, `16`, `32`, or `64`: little-endian immediate **after** `bytes` (if non-zero) |
| `reloc` | Optional `"pc32"`: requires `imm_bits` 32; operand is a symbol name; slot gets a PC-relative fixup |

Example:

```json
{ "mnemonic": "mov_rdi_r12", "bytes": "4c89e7", "imm_bits": 0 }
```

Anything not listed must use `.bytes` in the source.

### Assembly model

Processing is **single forward pass** over the source lines: labels bind to the current offset in the current section; `inst` and `.bytes` append to that section; relocations are recorded as they are emitted. There is **no** separate “collect all labels then emit” pass in the current implementation.

### API

```c
int kasm_compile(IsaSpec *isa, const char *input_path, const char *output_path, OobjObject *obj);
```

Returns `0` on success; on success the linker-visible data is written with `oobj_write_file` from `main.c`.

### Source language (subset)

- Line comments: `#` to end of line  
- `.section name align flags` — switch/create section  
- `.global name` / `.extern name` — symbols  
- `label:` — label at current offset  
- `inst mnemonic` or `inst mnemonic operand` — ISA-defined instruction  
- `.bytes aa bb …` — raw bytes  
- `.equ name, value` — numeric constant for later substitution in `inst` operands  

### Default ISA path

If `--isa` is omitted, `main.c` searches for `kasm/isa/x86_64.json` (see `--help` for search order and `OLANG_TOOLCHAIN_ROOT`).

### Output

One or more sections of machine code plus symbol and relocation tables in `.oobj` format (see [oobj spec](specs/oobj.md)).

---

[Return](README.md)
