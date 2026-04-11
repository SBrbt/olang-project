# Tests

**English** | **[中文](README_zh.md)**

---

### Run All Tests

```bash
make check
```

Runs, in order: link-script unit test (`bin/link_script_test`), alinker, kasm, preprocessor, then OLang integration (`examples/olc` over all `examples/programs/ex_*.ol`, `tests/olang/olang_*.ol`, multi-file link, plus expected-failure checks).

### Quick subset (OLang + examples only)

`tests/check_examples.sh` does `make clean && make all`, then only the two OLang scripts under `tests/olang/`. It does **not** run linker, kasm, or preproc tests — use `make check` before a release.

### Layout

```
tests/
├── README.md
├── check_examples.sh          # fast path: olang integration only (after full build)
├── link_script_test.c         # link script parser; Makefile builds bin/link_script_test
├── olang/
│   ├── run_programs_olc.sh    # ex_*.ol + tests/olang/olang_*.ol + multi_file link
│   ├── check_olang_bounds.sh  # expected compile failures (olang_fail/)
│   ├── olang_*.ol             # extra programs (paths listed in run_programs_olc.sh)
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
| **OLang** | `run_programs_olc.sh` compiles and runs every `ex_*.ol`, then `olang_*.ol` under `tests/olang/`, then multi-file link (see script output lines). `check_olang_bounds.sh` checks `olang_fail/*.ol` must not compile. |
| **alinker** | `smoke.sh`, `pc64.sh`, `multi_obj.sh` — fixture `.oobj` + JSON link scripts under `fixtures/`. |
| **kasm** | Assembler edge cases; temp dirs under `fixtures/`. |
| **preproc** | `include.sh` — `#include "..."` matches golden `expected.ol`. |
| **link script** | `link_script_test.c` exercises the C parser used by alinker. |

### Adding tests

1. New example program: add `examples/programs/ex_*.ol` (picked up by glob). For extra programs under `tests/olang/`, add the path to the `TESTS_OLANG_SRC` array in `run_programs_olc.sh` (must exit 0). Extend the same script if you need non-default exit code or stdout checks.
2. New component-level test: add a script under the matching subdirectory, put fixtures under `fixtures/<name>/`, add one `bash tests/...` line to the appropriate `check-*` rule in the top-level `Makefile`.

---

[Return to Docs](../docs/README.md)
