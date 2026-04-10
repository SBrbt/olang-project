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
- `bin/olang` — compiler
- `bin/kasm` — assembler
- `bin/alinker` — linker
- `examples/olc` — driver script

### Verify

```bash
make check
# OK: all checks passed
```

### First Program

Create `hello.ol`:

```olang
extern fn main() -> i32 {
    return 42;
}
```

Compile and run:

```bash
bash examples/olc -o hello.elf hello.ol
./hello.elf && echo $?
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
let x: i32 = 0i32;  // ✓
let y: i32;         // ✗ scalars must be initialized
```
