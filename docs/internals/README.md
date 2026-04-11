# Internals

**English** | **[中文](README_zh.md)**

---

Documentation for contributors.

### Quick Navigation

| Topic | Document |
|-------|----------|
| Architecture | [architecture.md](architecture.md) |
| Compiler | [compiler.md](compiler.md) |
| Assembler | [assembler.md](assembler.md) |
| Linker | [linker.md](linker.md) |
| Object Format | [specs/oobj.md](specs/oobj.md) |
| ELF Layout | [specs/elf-layout.md](specs/elf-layout.md) |
| Syntax Spec | [specs/olang-syntax.md](specs/olang-syntax.md) |
| Preprocessor | [preproc.md](preproc.md) |

### Source Map

```
olang/
├── src/frontend/
│   ├── lexer.c       # Lexical analysis
│   ├── parser.c      # Syntax analysis
│   └── sema.c        # Semantic analysis
└── src/backend/
    ├── x64_backend.c   # x86_64 backend entry
    ├── codegen_x64.c   # Code generation
    └── reloc/          # Relocations

kasm/
├── src/
│   ├── kasm_asm.c    # Assembler
│   └── kasm_isa.c    # ISA parser
└── isa/
    └── x86_64_linux.json  # Instruction set definition

alinker/
└── src/
    ├── link_core.c     # Linking core
    └── main.c          # CLI

preproc/
└── src/
    └── main.c          # olprep (#include only)
```

### Development Workflow

1. **Modify** → `olang/src/`, `kasm/src/`, etc.
2. **Build** → `make all`
3. **Test** → `make check`
4. **Verify** → `bash tests/olang/run_programs_olc.sh`

---

**Quick Links**: [Docs Center](../README.md) | [Project Home](../../README.md)
