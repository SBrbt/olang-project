# Examples

**English** | **[中文](README_zh.md)**

---

### Directory Layout

| Path | Purpose |
|------|---------|
| [`programs/`](programs/) | `ex_*.ol` single-file tests; `multi_file_main.ol` / `multi_file_lib.ol` two-unit linking |
| [`linux_x86_64/`](linux_x86_64/) | Linux x86_64 platform package: `link/*.json`, `asm/**` |
| [`olc`](olc) | Bash driver (invokes `bin/olang`, `kasm`, `alinker`) |
| `out/` | Build output directory (do not commit) |

### Quick Start

```bash
make all
bash examples/olc -o examples/out/hello.elf examples/programs/ex_hello.ol
./examples/out/hello.elf
```

### Expected Results

Exit codes and stdout expectations are defined in [`tests/olang/run_programs_olc.sh`](../tests/olang/run_programs_olc.sh) (`make check` runs it); the table below is for quick reference — when in doubt, the script is authoritative.

| Program (`ex_*.ol`) | Expected |
|----------------------|----------|
| `ex_hello` | Exit `0`, stdout contains `Hello from OLang` |
| `ex_write_ok` | Exit `0`, stdout contains `OK` |
| `ex_stdout_two` | Exit `0`, stdout contains `AB` |
| All other `ex_*.ol` (including `ex_rt_*`) | Exit `0` (self-check inside `main`) |

### Robustness Tests (`ex_rt_*`)

- Prefix **`ex_rt_`**: compiler/backend regression scenarios.
- Default **`return cast<i32>(0)`** means pass.
- Phase 1 has no reassignment to existing `let`; counting scenarios may use recursion instead of mutable loops.

Adding a new `ex_*.ol` causes the test script to automatically pick it up; edit `tests/olang/run_programs_olc.sh` for custom stdout/exit assertions.

### Multi-file Example

```bash
bash examples/olc -o examples/out/multi.elf \
    examples/programs/multi_file_main.ol \
    examples/programs/multi_file_lib.ol
./examples/out/multi.elf; echo $?
# 42
```

Each `.ol` is compiled to `.oobj`, then linked with the runtime. See [docs/learn/tutorial.md](../docs/learn/tutorial.md) for details.

---

[Return to Docs](../docs/README.md)
