# Tests

**English** | **[中文](README_zh.md)**

---

### Run All Tests

```bash
make check
```

### Test Structure

```
tests/
├── README.md                 # This file
├── run_programs_olc.sh       # 30+ example programs test
├── check_olang_bounds.sh     # Edge case tests
├── alinker_*.sh              # Linker tests
├── kasm_*.sh                 # Assembler tests
├── fixtures/                 # Test fixtures
├── olang_*.ol                # Edge case test files
└── olang_fail/               # Expected-failure tests
```

### Test Categories

#### Regression Tests

`run_programs_olc.sh` — Compile and run all `examples/programs/ex_*.ol`:

```bash
bash tests/run_programs_olc.sh
# OK: run_programs_olc.sh (30 programs)
```

#### Edge Case Tests

`check_olang_bounds.sh` — Test compiler edge cases:

| Test | Description |
|------|-------------|
| `olang_aggregate_copy.ol` | Aggregate value copy |
| `olang_nested_struct.ol` | Nested struct access |
| `olang_u64_max.ol` | u64 maximum range |

#### Failure Tests

`olang_fail/` — Programs expected to fail compilation:

- `fail_long_ident.ol` — Identifier too long
- `fail_unterminated_string.ol` — Unterminated string

#### Linker Tests

- `alinker_smoke.sh` — Basic functionality
- `alinker_pc64.sh` — PC-relative addressing
- `alinker_multi_obj.sh` — Multi-object linking

#### Assembler Tests

- `kasm_label_comment.sh` — Labels and comments
- `kasm_bytes_tab.sh` — Byte tables

### Adding New Tests

1. Create test program `tests/my_test.ol`
2. Add to existing test script or create new one
3. Reference in `Makefile` `check` target

---

[Return to Docs](../docs/README.md)
