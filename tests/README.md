# Tests

**English** | **[中文](README_zh.md)**

---

### Run All Tests

```bash
make check
```

Runs, in order: link-script unit test (`bin/link_script_test`), Markdown `.ol` link check (`python3 scripts/verify_doc_olang_refs.py`), alinker, kasm, preprocessor, then OLang integration (`examples/linux_x86_64/olc` over all `examples/linux_x86_64/programs/ex_*.ol`, multi-file link, plus expected-failure checks).

### Quick subset (OLang + examples only)

`tests/check_examples.sh` does `make clean && make all`, then only the two OLang scripts under `tests/olang/`. It does **not** run linker, kasm, or preproc tests — use `make check` before a release.

### Layout

```
tests/
├── README.md
├── check_examples.sh          # fast path: olang integration only (after full build)
├── link_script_test.c         # link script parser; Makefile builds bin/link_script_test
├── olang/
│   ├── run_programs_olc.sh    # all ex_*.ol + multi_file link
│   ├── check_olang_bounds.sh  # expected compile failures (olang_fail/)
│   └── olang_fail/            # inputs that must fail compilation
├── alinker/
│   ├── smoke.sh
│   ├── pc64.sh
│   └── multi_obj.sh
├── kasm/
│   ├── label_comment.sh
│   └── bytes_tab.sh
├── preproc/
│   └── include.sh             # olprep #include
└── fixtures/                  # shared data for the scripts above
```

### By component

| Area | Role |
|------|------|
| **OLang** | `run_programs_olc.sh` compiles and runs every `examples/linux_x86_64/programs/ex_*.ol`, then multi-file link (see script output lines). `check_olang_bounds.sh` checks `olang_fail/*.ol` must not compile. |
| **alinker** | `smoke.sh`, `pc64.sh`, `multi_obj.sh` — fixture `.oobj` + JSON link scripts under `fixtures/`. |
| **kasm** | Assembler edge cases; temp dirs under `fixtures/`. |
| **preproc** | `include.sh` — `#include "..."` matches golden `expected.ol`. |
| **link script** | `link_script_test.c` exercises the C parser used by alinker. |

### Adding tests

1. New example program: add `examples/linux_x86_64/programs/ex_*.ol` (picked up by glob). Extend `run_programs_olc.sh` if you need non-default exit code or stdout checks (see `ex_hello` / `ex_write_ok` / `ex_stdout_two` branches).
2. New component-level test: add a script under the matching subdirectory, put fixtures under `fixtures/<name>/`, add one `bash tests/...` line to the appropriate `check-*` rule in the top-level `Makefile`.

---

[Return to Docs](../docs/README.md)
