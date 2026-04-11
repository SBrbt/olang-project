# 示例

**[English](examples.md)** | **中文**

---

30+ 个可运行示例。

### POSIX 封装（`libposix.kasm`）

共用的 `extern fn posix_*` 声明在 [`examples/include/posix_abi.ol`](../../examples/include/posix_abi.ol)。示例里用 `#include "posix_abi.ol"`；`examples/olc` 会传入 `-I` 以解析该路径。

### 快速运行

```bash
# 运行任意示例
bash examples/olc -o examples/out/t.elf examples/programs/EXAMPLE.ol
./examples/out/t.elf

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
| `ex_rt_load_store_i32.ol` | 内存操作 |

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

#### 聚合类型

| 示例 | 说明 |
|------|------|
| `ex_rt_local_array.ol` | 数组 |

#### 多文件

| 示例 | 说明 |
|------|------|
| `multi_file_main.ol` + `multi_file_lib.ol` | 多文件编译 |

```bash
bash examples/olc -o examples/out/multi.elf \
    examples/programs/multi_file_main.ol \
    examples/programs/multi_file_lib.ol
```

### 边界测试

位于 `tests/olang/`（`make check` 里会经 `tests/olang/run_programs_olc.sh` 编译并运行）：

- `olang_aggregate_copy.ol` — 聚合类型值拷贝
- `olang_nested_struct.ol` — 嵌套结构体访问
- `olang_u64_max.ol` — u64 最大范围

---

[返回文档](../README_zh.md)
