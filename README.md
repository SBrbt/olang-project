# OLang

**English** | **[中文](README_zh.md)**

---

A statically typed systems programming language built from scratch.

```bash
make all
bash examples/olc -o examples/out/hello.elf examples/programs/ex_hello.ol
./examples/out/hello.elf
```

**[Get Started →](docs/README.md)**

### One-liner

OLang is a compiled language that generates ELF directly with zero external dependencies.

### Design stance

OLang **deliberately** exposes low-level control (memory layout, raw pointers, reinterpreting bit patterns) instead of hiding the machine model. The compiler enforces the type system and many static rules, but **runtime safety**—avoiding out-of-bounds access, use-after-free, and other undefined behavior—is **your responsibility as a low-level developer**. The language does not pretend to guarantee the same things a managed or proof-carrying runtime would.

### Documentation Map

| I want to... | Go here |
|--------------|---------|
| Get started in 5 minutes | [docs/learn/quickstart.md](docs/learn/quickstart.md) |
| Learn the language | [docs/learn/tutorial.md](docs/learn/tutorial.md) |
| Look up syntax/features | [docs/book/](docs/book/) |
| See examples | [docs/learn/examples.md](docs/learn/examples.md) |
| Understand implementation | [docs/internals/](docs/internals/) |
| Contribute | [docs/internals/README.md](docs/internals/README.md) |

### Project Status

Phase 1 (core features complete): type system, functions, structs, arrays, control flow, multi-file compilation — [see tests](tests/README.md)

### Directory Structure

```
├── docs/          # All documentation
├── olang/         # Compiler source
├── preproc/       # Preprocessor (olprep)
├── kasm/          # Assembler
├── alinker/       # Linker
├── common/        # Shared code
├── examples/      # Example programs
├── tests/         # Test suite
└── bin/           # Build output
```

**License**: [MIT](LICENSE)
