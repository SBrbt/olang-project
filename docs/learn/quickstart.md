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
- `examples/olc` — driver script (runs `olprep` then the rest; keeps intermediates in `<output>.olc.d/`)

### Verify

```bash
make check
# OK: all checks passed
```

### First Program

Create `hello.ol` in `examples/programs`:

```olang
extern i32 main() {
    return 42;
}
```

Compile and run:

```bash
mkdir -p examples/out
bash examples/olc -o examples/out/hello.elf examples/programs/hello.ol
./examples/out/hello.elf
echo $?
# 42
```

### Next Steps

- Full tutorial: [tutorial.md](tutorial.md)
- More examples: [examples.md](examples.md)
- Syntax reference: [../book/syntax.md](../book/syntax.md)

### FAQ

**Q: `make check` fails?**
Check GCC version: `gcc --version` (requires 4.8+)

**Q: Cannot find `olc`?**
Ensure you're in project root, use: `bash examples/olc`

**Q: "scalar needs initializer"?**
```olang
let x<i32> @stack<32>(0i32);  // ✓
// let y<i32> @stack<32>();   // ✗ scalars must be initialized
```
