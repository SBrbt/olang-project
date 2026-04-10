# 测试

**[English](README.md)** | **中文**

---

### 运行全部测试

```bash
make check
```

### 测试结构

```
tests/
├── README.md                 # 本文件
├── run_programs_olc.sh       # 30+ 示例程序测试
├── check_olang_bounds.sh     # 边界测试
├── alinker_*.sh              # 链接器测试
├── kasm_*.sh                 # 汇编器测试
├── fixtures/                 # 测试固件
├── olang_*.ol                # 边界测试用例
└── olang_fail/               # 预期失败的测试
```

### 测试类别

#### 回归测试

`run_programs_olc.sh` — 编译并运行所有 `examples/programs/ex_*.ol`：

```bash
bash tests/run_programs_olc.sh
# OK: run_programs_olc.sh (30 programs)
```

#### 边界测试

`check_olang_bounds.sh` — 测试编译器边界情况：

| 测试 | 说明 |
|------|------|
| `olang_aggregate_copy.ol` | 聚合类型值拷贝 |
| `olang_nested_struct.ol` | 嵌套结构体访问 |
| `olang_u64_max.ol` | u64 最大范围 |

### 失败测试

`olang_fail/` — 预期编译失败的程序：

- `fail_long_ident.ol` — 标识符过长
- `fail_unterminated_string.ol` — 未终止字符串

### 链接器测试

- `alinker_smoke.sh` — 基本功能
- `alinker_pc64.sh` — 程序计数器相对寻址
- `alinker_multi_obj.sh` — 多对象链接

### 汇编器测试

- `kasm_label_comment.sh` — 标签与注释
- `kasm_bytes_tab.sh` — 字节表

### 添加新测试

1. 创建测试程序 `tests/my_test.ol`
2. 添加测试脚本或添加到现有脚本
3. 在 `Makefile` 的 `check` 目标中引用

---

[返回文档](../docs/README_zh.md)
