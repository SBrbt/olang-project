# Examples

**English** | **[中文](examples_zh.md)**

---

Many runnable examples (`examples/linux_x86_64/programs/ex_*.ol`, exercised by `make check`).

### POSIX syscall shims (`libposix.kasm`)

Shared `extern` POSIX declarations (e.g. `posix_write(fd: i32, buf: ptr, n: u64)`) live in [`examples/linux_x86_64/include/posix_abi.ol`](../../examples/linux_x86_64/include/posix_abi.ol). Example programs use `#include "posix_abi.ol"`; `examples/linux_x86_64/olc` passes `-I` so that path resolves.

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
| `ex_rt_multi_view.ol` | `struct` with `f32` / `i32` fields on one `stack[64, …]` slot |
| `ex_rt_u32_view.ol` | Same 4-byte slot: `i32` value read as `u32` via `<u32>x` |
| `ex_rt_global_sections.ol` | Globals and sections (`data` / `bss` / `rodata`) |
| `ex_rt_global_multi_view.ol` | Same as `ex_rt_multi_view.ol` (function-local `struct` + `stack`) |
| `ex_rt_section_custom.ol` | Custom section: `section["…", bitwidth, init]` |
| `ex_rt_f16.ol` | `f16` literals and casts (combine with `f32` for arithmetic) |
| `ex_rt_sizeof.ol` | `sizeof[Type]` as `u64`; `stack[bitwidth, …]` placement |
| `ex_rt_stack_infer.ol` | `stack[32, expr]`, `rodata[32, …]` — explicit bit width + initializer |
| `ex_rt_float_sci.ol` | Float literals with exponent (`1e2`, `1e-3f32`, …) |
| `ex_rt_find_wrapped.ol` | `find[…]` with extra parentheses around the ptr operand |

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
| `ex_rt_b8_bitwise.ol` | `b8` (avoid `0b8` token clash; use e.g. `1b8 ^ 1b8` for zero, not a same-width `u8`→`b8` cast) |

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
