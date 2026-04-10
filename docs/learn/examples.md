# Examples

**English** | **[中文](examples_zh.md)**

---

30+ runnable examples.

### Quick Run

```bash
# Run any example
bash examples/olc -o examples/out/t.elf examples/programs/EXAMPLE.ol
./examples/out/t.elf

# Run all
make check
```

### Index by Category

#### Basics

| Example | Description |
|---------|-------------|
| `ex_hello.ol` | Hello World |
| `ex_write_ok.ol` | Basic write syscall |
| `ex_stdout_two.ol` | Sequential output |

#### Variables and Types

| Example | Description |
|---------|-------------|
| `ex_rt_int_suffix.ol` | Integer suffixes |
| `ex_rt_hex_literal.ol` | Hexadecimal |
| `ex_rt_unsigned_types.ol` | Unsigned arithmetic |
| `ex_rt_neg.ol` | Unary negation |
| `ex_rt_load_store_i32.ol` | Memory operations |

#### Control Flow

| Example | Description |
|---------|-------------|
| `ex_rt_if_else.ol` | Conditional |
| `ex_rt_while_count.ol` | Loop |
| `ex_rt_break_continue.ol` | Loop control |
| `ex_rt_bool_logic.ol` | Boolean logic |

#### Functions

| Example | Description |
|---------|-------------|
| `ex_rt_deep_recursion.ol` | Recursion |
| `ex_rt_six_args.ol` | 6 parameters |

#### Aggregate Types

| Example | Description |
|---------|-------------|
| `ex_rt_local_array.ol` | Arrays |

#### Multi-file

| Example | Description |
|---------|-------------|
| `multi_file_main.ol` + `multi_file_lib.ol` | Multi-file compilation |

```bash
bash examples/olc -o examples/out/multi.elf \
    examples/programs/multi_file_main.ol \
    examples/programs/multi_file_lib.ol
```

### Edge Case Tests

Located in `tests/`:

- `olang_aggregate_copy.ol` — aggregate value copy
- `olang_nested_struct.ol` — nested struct access
- `olang_u64_max.ol` — u64 maximum range

---

[Return to Docs](../README.md)
