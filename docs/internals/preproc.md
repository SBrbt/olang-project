# Preprocessor (olprep)

**English** | **[中文](preproc_zh.md)**

---

`bin/olprep` is a **small, language-agnostic** line-oriented preprocessor. It is **not** part of the OLang grammar; the compiler only sees the output of `olprep`.

### Role in the toolchain

`examples/olc` runs, for each source `.ol`:

1. `olprep --in <source> -o <workdir>/ppN.ol -I <repo>/examples/include`
2. `olang --in <workdir>/ppN.ol -o <workdir>/uN.oobj`

The default `-I` lets sources use `#include "posix_abi.ol"` for `extern` declarations matching [`libposix.kasm`](../../examples/linux_x86_64/asm/lib/libposix.kasm) (see [`examples/include/posix_abi.ol`](../../examples/include/posix_abi.ol)).
3. … then `kasm` / `alinker` as before.

### Implemented directives

- `#include "path"` — insert the full text of the resolved file at this line. Relative paths are resolved against the **directory of the file containing the `#include`**, then each `-I` directory in order. Absolute paths are allowed.

Other `#` directives are not supported.

### CLI

```
olprep --in <file> -o <out> [-I <dir>]...
```

### Safety

- **Circular includes** are detected (same canonical path on the include stack) and reported as an error.
- **Maximum include depth** is 64.

### Diagnostics and line numbers

There is **no** `#line` directive and **no** mapping of diagnostics back to pre-expansion line numbers. Compiler errors refer to lines in the **preprocessed** file (e.g. `pp0.ol` under the work directory). Inspect that file when debugging.

### `olc` work directory

For `-o /path/to/foo.elf`, intermediates are written to:

`/path/to/foo.olc.d/`

That directory is **not** removed by `olc` (preprocessed `.ol`, `.oobj`, and assembled runtime objects are kept).

---

[Return](README.md)
