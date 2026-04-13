# Examples

**English** | **[中文](README_zh.md)**

---

### Directory layout

| Path | Purpose |
|------|---------|
| [`linux_x86_64/`](linux_x86_64/) | **Linux x86_64** platform package: [`olc`](linux_x86_64/olc), [`programs/`](linux_x86_64/programs/), [`include/`](linux_x86_64/include/), [`link/linux_elf_exe.json`](linux_x86_64/link/linux_elf_exe.json), [`asm/`](linux_x86_64/asm/) |
| [`bare_x86_64/`](bare_x86_64/) | **Bare-metal x86_64** link-script example only (flat raw image; not Linux-specific) |

Build artifacts for the Linux driver go under `linux_x86_64/out/` (gitignored; created by tests or local runs).

### Quick start (Linux ELF)

```bash
make all
bash examples/linux_x86_64/olc -o examples/linux_x86_64/out/hello.elf examples/linux_x86_64/programs/ex_hello.ol
./examples/linux_x86_64/out/hello.elf
```

### Expected results

Exit codes and stdout checks are defined in [`tests/olang/run_programs_olc.sh`](../tests/olang/run_programs_olc.sh) (`make check` runs it); the table in this repo’s learn docs is for quick reference — the script is authoritative.

| Program (`ex_*.ol`) | Expected |
|----------------------|----------|
| `ex_hello` | Exit `0`, stdout contains `Hello from OLang` |
| `ex_write_ok` | Exit `0`, stdout contains `OK` |
| `ex_stdout_two` | Exit `0`, stdout contains `AB` |
| All other `ex_*.ol` (including `ex_rt_*`) | Exit `0` (self-check in `main`) |

### Robustness tests (`ex_rt_*`)

- Prefix **`ex_rt_`**: compiler/backend regression scenarios.
- Default **`return i32(0)`** means pass.

Adding a new `ex_*.ol` under `linux_x86_64/programs/` is picked up automatically; edit `tests/olang/run_programs_olc.sh` for custom stdout/exit assertions.

### Multi-file example

```bash
bash examples/linux_x86_64/olc -o examples/linux_x86_64/out/multi.elf \
    examples/linux_x86_64/programs/multi_file_main.ol \
    examples/linux_x86_64/programs/multi_file_lib.ol
./examples/linux_x86_64/out/multi.elf; echo $?
# 42
```

See [docs/learn/tutorial.md](../docs/learn/tutorial.md) for details.

---

[Return to Docs](../docs/README.md)
