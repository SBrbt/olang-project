# Quickstart

**English** | **[中文](quickstart_zh.md)**

---

Get OLang running in 5 minutes.

### Requirements

- Linux x86_64
- GCC (C11)
- Make

### Build

```bash
make all
```

Generates:
- `bin/olprep` — preprocessor (`#include` only)
- `bin/olang` — compiler
- `bin/kasm` — assembler
- `bin/alinker` — linker
- `examples/linux_x86_64/olc` — driver script (runs `olprep` then the rest; keeps intermediates in `<output>.olc.d/`)

### Verify

```bash
make check
# OK: all checks passed
```

### First Program

Create a local playground directory (ignored by git — see repo `.gitignore`) and put your source next to the ELF:

```bash
mkdir -p examples/linux_x86_64/programs/hello
```

Create `examples/linux_x86_64/programs/hello/hello.ol`:

```olang
extern i32 main() {
    return 42;
}
```

Compile and run from the repo root (output stays under `hello/`):

```bash
bash examples/linux_x86_64/olc -o examples/linux_x86_64/programs/hello/hello.elf examples/linux_x86_64/programs/hello/hello.ol
./examples/linux_x86_64/programs/hello/hello.elf
echo $?
# 42
```

`make check` only compiles `examples/linux_x86_64/programs/ex_*.ol` by convention; this `hello/` tree is for your own experiments.

### Next Steps

- Full tutorial: [tutorial.md](tutorial.md)
- More examples: [examples.md](examples.md)
- Syntax reference: [../book/syntax.md](../book/syntax.md)

### FAQ

**Q: `make check` fails?**
Check GCC version: `gcc --version` (requires 4.8+)

**Q: Cannot find `olc`?**
Ensure you're in project root, use: `bash examples/linux_x86_64/olc`

**Q: "scalar needs initializer"?**
```olang
let x stack[32, 0i32];  // ✓
// let y stack[32];     // ✗ need an initializer, or: let raw stack[32]; let y <i32> raw;
```
