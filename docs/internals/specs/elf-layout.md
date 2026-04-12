# Example: ELF-shaped output (platform package)

**English** | **[中文](elf-layout_zh.md)**

---

### Overview

`alinker` itself does **not** “output ELF” as a built-in: it writes bytes according to the link script’s `layout` array. The repository’s **Linux x86_64 ELF executable** example (`examples/linux_x86_64/link/linux_elf_exe.json`) uses `write_blob` steps that match a **64-bit ELF** header and program headers, then `write_payload` for the merged image. For contrast, `examples/bare_x86_64/link/raw.bin.json` is a **bare-metal-style** flat image: no container bytes, no prepend stub, payload at file offset 0 (see `examples/bare_x86_64/README.md`).

### Layout (example `linux_elf_exe.json`)

```
┌─────────────────────┐ 0x0
│ Bytes from layout   │  (ELF-like header + Phdrs from script hex)
├─────────────────────┤
│ Payload @ 0x1000    │  stub (from call_stub_hex) + sections
│   .text / .rodata … │
└─────────────────────┘
```

### Segments (example)

Typical `linux_elf_exe.json` uses two load segments (RX then RW); exact `file_off` / `vaddr` / flags come from the script.

### Entry symbol

The symbol used as the logical entry for patching (`"entry"`) is **only** the string in the link script. It is **not** chosen by the OLang compiler.

```json
{
  "entry": "_start"
}
```

### Entry stub (`call_stub_hex`)

When `"prepend_call_stub"` is true, the script **must** supply **`call_stub_hex`**: raw bytes placed before merged section data. `alinker` does not ship a default stub; it only copies these bytes and, if the first byte is `0xE8`, patches the rel32 at +1 to `call` the `"entry"` symbol.

The **example** `linux_elf_exe.json` uses a 14-byte stub that (on x86-64 Linux) corresponds to: `call entry` → then use `main`’s return in `eax` and issue **exit** via syscall 60 — but that semantics lives in the **script hex**, not in the linker.

### Runtime objects (`examples/linux_x86_64/olc`)

Typical inputs:

- `krt.kasm.oobj` — defines `_start` (`krt_init` → `main` → `krt_fini`)
- `olrt.kasm.oobj` — compiler helpers; **OLang ABI** per `olrt.kasm`
- `libposix.kasm.oobj` — syscall shims (`syscall` instruction), not libc

Execution order when using the example stub + `krt`:

1. Kernel loads the process; ELF entry (from the script-written header) points at the payload start (stub + …).
2. Stub runs (bytes from `call_stub_hex`), then returns or transfers per those bytes.
3. `_start` runs as defined in `krt.kasm`.

---

[Return](../README.md)
