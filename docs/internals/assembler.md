# Assembler

**English** | **[中文](assembler_zh.md)**

---

Location: `kasm/`

### Structure

```
kasm/
├── src/
│   ├── main.c          # Entry point
│   ├── kasm_asm.c/h    # Assembly core
│   └── kasm_isa.c/h    # ISA parser
└── isa/
    └── x86_64_linux.json  # Instruction set definition
```

### ISA Definition

`kasm/isa/x86_64_linux.json` defines:
- Instruction encoding formats
- Operand types
- Register encodings

Example entry:
```json
{
  "mnemonic": "mov",
  "operands": ["r64", "r64"],
  "encoding": ["0x48", "0x89", "/r"]
}
```

### Assembly Process

Two-pass assembly:

1. **First pass** — Collect symbols (label addresses)
2. **Second pass** — Generate code (parse instructions, apply relocations)

### Key Functions

```c
// Main assembly function
int kasm_assemble(KasmState *S, const char *input, size_t len);

// ISA loading
int isa_load(ISA *isa, const char *path);
```

### Input Format

```kasm
; comment
label:
    mov rax, 42
    jmp label
```

### Output

`.oobj` object file:
- Code section
- Symbol table
- Relocation table

---

[Return](README.md)
