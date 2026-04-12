# 示例

**[English](examples.md)** | **中文**

---

64+ 个可运行示例（`examples/linux_x86_64/programs/ex_*.ol`，由 `make check` 批量运行）。

### POSIX 封装（`libposix.kasm`）

共用的 `extern` POSIX 声明（如 `extern i64 posix_write(...);`）在 [`examples/linux_x86_64/include/posix_abi.ol`](../../examples/linux_x86_64/include/posix_abi.ol)。示例里用 `#include "posix_abi.ol"`；`examples/linux_x86_64/olc` 会传入 `-I` 以解析该路径。

### 快速运行

```bash
# 运行任意示例
bash examples/linux_x86_64/olc -o examples/linux_x86_64/out/t.elf examples/linux_x86_64/programs/EXAMPLE.ol
./examples/linux_x86_64/out/t.elf

# 运行全部
make check
```

### 分类索引

#### 基础

| 示例 | 说明 |
|------|------|
| `ex_hello.ol` | Hello World |
| `ex_write_ok.ol` | 基础 write 调用 |
| `ex_stdout_two.ol` | 顺序输出 |

#### 变量与类型

| 示例 | 说明 |
|------|------|
| `ex_rt_int_suffix.ol` | 整数后缀 |
| `ex_rt_hex_literal.ol` | 十六进制 |
| `ex_rt_unsigned_types.ol` | 无符号运算 |
| `ex_rt_neg.ol` | 一元负号 |
| `ex_rt_load_store_i32.ol` | 经 `addr` 的 `load`/`store`（`i32`） |
| `ex_rt_load_store_widths.ol` | `i8`、`i16`、`i64`、`u32` 的 `load`/`store` |
| `ex_rt_ptr_eq.ol` | `ptr` 的 `==` / `!=` |
| `ex_rt_multi_view.ol` | 栈上多绑定：`f32` 与 `i32` 两种视图，分别做浮点与整数运算 |
| `ex_rt_u32_view.ol` | 同 4 字节：`i32` 经 `<u32>addr` 当作 `u32` 读 |
| `ex_rt_global_sections.ol` | 全局变量与各段（`@data` / `@bss` / `@rodata`） |
| `ex_rt_global_multi_view.ol` | 文件级多绑定：与 `ex_rt_multi_view.ol` 相同的 `f32`/`i32` 布局 |
| `ex_rt_section_custom.ol` | 自定义全局段名 `@section("…")` |
| `ex_rt_f16.ol` | `f16` 字面量与转换（与 `f32` 配合做算术） |

#### 控制流

| 示例 | 说明 |
|------|------|
| `ex_rt_if_else.ol` | 条件语句 |
| `ex_rt_while_count.ol` | 循环 |
| `ex_rt_break_continue.ol` | 循环控制 |
| `ex_rt_bool_logic.ol` | 布尔逻辑 |

#### 函数

| 示例 | 说明 |
|------|------|
| `ex_rt_deep_recursion.ol` | 递归 |
| `ex_rt_six_args.ol` | 6个参数 |
| `ex_rt_eight_args.ol` | 8个寄存器参数 |
| `ex_rt_many_args.ol` | 10个参数（寄存器+栈） |
| `ex_rt_extern_forward.ol` | 前置 `extern` 声明再定义 |

#### 位向量（`b8`…`b64`）

| 示例 | 说明 |
|------|------|
| `ex_rt_binary_ops.ol` | `b32` 移位、位运算与算术 |
| `ex_rt_b16_b64.ol` | `b16` / `b64` |
| `ex_rt_b8_bitwise.ol` | `b8`（字面量零勿写 `0b8`，会与二进制前缀冲突；可用 `0b8` 或 `cast<b8>(0u8)`） |

#### 聚合类型

| 示例 | 说明 |
|------|------|
| `ex_rt_local_array.ol` | 标量数组 |
| `ex_rt_array_of_struct.ol` | 元素类型为 `struct` 的数组 |
| `ex_rt_struct_copy.ol` | 结构体赋值（值拷贝） |
| `ex_rt_aggregate_copy.ol` | 同上（另一套字段布局） |
| `ex_rt_nested_struct.ol` | 嵌套结构体字段 |
| `ex_rt_u64_max.ol` | `u64` 最大字面量与加法回绕 |

#### 多文件

| 示例 | 说明 |
|------|------|
| `multi_file_main.ol` + `multi_file_lib.ol` | 多文件编译 |

```bash
bash examples/linux_x86_64/olc -o examples/linux_x86_64/out/multi.elf \
    examples/linux_x86_64/programs/multi_file_main.ol \
    examples/linux_x86_64/programs/multi_file_lib.ol
```

### 编译器边界检查

`tests/olang/check_olang_bounds.sh`（`make check` 的一部分）对 `tests/olang/olang_fail/` 下的输入做**预期编译失败**检查（如标识符过长、错误字符串等）。

---

[返回文档](../README_zh.md)
