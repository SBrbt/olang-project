# Examples

**English** | **[中文](examples_zh.md)**

---

64+ runnable examples (`examples/linux_x86_64/programs/ex_*.ol`, exercised by `make check`).

### POSIX syscall shims (`libposix.kasm`)

Shared `extern` POSIX declarations (e.g. `extern i64 posix_write(...);`) live in [`examples/linux_x86_64/include/posix_abi.ol`](../../examples/linux_x86_64/include/posix_abi.ol). Example programs use `#include "posix_abi.ol"`; `examples/linux_x86_64/olc` passes `-I` so that path resolves.

### Quick Run

```bash
# Run any example
bash examples/linux_x86_64/olc -o examples/linux_x86_64/out/t.elf examples/linux_x86_64/programs/EXAMPLE.ol
./examples/linux_x86_64/out/t.elf

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
| `ex_rt_load_store_i32.ol` | `load`/`store` via `addr` (`i32`) |
| `ex_rt_load_store_widths.ol` | `load`/`store` for `i8`, `i16`, `i64`, `u32` |
| `ex_rt_ptr_eq.ol` | `ptr` `==` / `!=` |
| `ex_rt_multi_view.ol` | Multi-binding on stack (several types, one blob) |
| `ex_rt_global_sections.ol` | Globals and sections (`@data` / `@bss` / `@rodata`) |
| `ex_rt_global_multi_view.ol` | File-scope multi-binding (same rules as stack) |
| `ex_rt_section_custom.ol` | Custom global section name `@section("…")` |
| `ex_rt_f16.ol` | `f16` literals and casts (combine with `f32` for arithmetic) |

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
| `ex_rt_eight_args.ol` | 8 register parameters |
| `ex_rt_many_args.ol` | 10 parameters (register + stack) |
| `ex_rt_extern_forward.ol` | Forward `extern` prototype, then definition |

#### Bit vectors (`b8`…`b64`)

| Example | Description |
|---------|-------------|
| `ex_rt_binary_ops.ol` | `b32` shifts / bitwise / arithmetic |
| `ex_rt_b16_b64.ol` | `b16` / `b64` |
| `ex_rt_b8_bitwise.ol` | `b8` (avoid `0b8` token; use `reinterpret<b8>(0u8)` for zero) |

#### Aggregate Types

| Example | Description |
|---------|-------------|
| `ex_rt_local_array.ol` | Array of scalars |
| `ex_rt_array_of_struct.ol` | Array whose element type is a `struct` |
| `ex_rt_struct_copy.ol` | Struct assignment (value copy) |
| `ex_rt_aggregate_copy.ol` | Same semantics (alternate layout) |
| `ex_rt_nested_struct.ol` | Nested struct fields |
| `ex_rt_u64_max.ol` | `u64` max literal and wrap |

#### Multi-file

| Example | Description |
|---------|-------------|
| `multi_file_main.ol` + `multi_file_lib.ol` | Multi-file compilation |

```bash
bash examples/linux_x86_64/olc -o examples/linux_x86_64/out/multi.elf \
    examples/linux_x86_64/programs/multi_file_main.ol \
    examples/linux_x86_64/programs/multi_file_lib.ol
```

### Compiler bounds checks

`tests/olang/check_olang_bounds.sh` (part of `make check`) compiles inputs under `tests/olang/olang_fail/` and expects **failure** (e.g. overlong identifier, bad string).

---

[Return to Docs](../README.md)
