# 示例

**[English](examples.md)** | **中文**

---

30+ 个可运行示例。

### 快速运行

```bash
# 运行任意示例
bash examples/olc -o /tmp/t.elf examples/programs/EXAMPLE.ol
/tmp/t.elf

# 运行全部
bash tests/run_programs_olc.sh
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

#### 聚合类型

| 示例 | 说明 |
|------|------|
| `ex_rt_local_array.ol` | 数组 |
| `olang_aggregate_copy.ol` | 值拷贝（测试） |
| `olang_nested_struct.ol` | 嵌套结构体（测试） |

#### 多文件

| 示例 | 说明 |
|------|------|
| `multi_file_main.ol` + `multi_file_lib.ol` | 多文件编译 |

```bash
bash examples/olc -o /tmp/t.elf \
    examples/programs/multi_file_main.ol \
    examples/programs/multi_file_lib.ol
```

### 边界测试

位于 `tests/olang_*.ol`：

- `olang_aggregate_copy.ol` — 聚合类型值拷贝
- `olang_nested_struct.ol` — 嵌套结构体访问
- `olang_u64_max.ol` — u64 最大范围

---

[返回文档](../README_zh.md)
