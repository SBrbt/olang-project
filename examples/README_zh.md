# 示例

**[English](README.md)** | **中文**

---

### 目录结构

| 路径 | 作用 |
|------|------|
| [`programs/`](programs/) | `ex_*.ol` 单文件测试；`multi_file_main.ol` / `multi_file_lib.ol` 双单元链接 |
| [`linux_x86_64/`](linux_x86_64/) | Linux x86_64 平台包：`link/*.json`、`asm/**` |
| [`olc`](olc) | Bash 驱动（调用 `bin/olang`、`kasm`、`alinker`） |
| `out/` | 产物目录（勿提交） |

### 快速开始

```bash
make all
bash examples/olc -o examples/out/hello.elf examples/programs/ex_hello.ol
./examples/out/hello.elf
```

### 期望结果

退出码与 stdout 以 [`tests/run_programs_olc.sh`](../tests/run_programs_olc.sh) 为准（`make check` 会执行）；下表便于阅读，不一致时以脚本为准。

| 程序 (`ex_*.ol`) | 期望 |
|------------------|------|
| `ex_hello` | 退出码 `0`，stdout 含 `Hello from OLang` |
| `ex_write_ok` | 退出码 `0`，stdout 含 `OK` |
| `ex_stdout_two` | 退出码 `0`，stdout 含 `AB` |
| 其余 `ex_*.ol`（含 `ex_rt_*`） | 退出码 `0`（`main` 内自检） |

### 鲁棒性用例（`ex_rt_*`）

- 前缀 **`ex_rt_`**：编译器/后端回归场景。
- 默认 **`return cast<i32>(0)`** 表示通过。
- Phase 1 无对已有 `let` 的赋值；计数场景可能用递归代替可变循环。

新增 `ex_*.ol` 时脚本会自动编译运行；需要特殊 stdout/退出码断言时编辑 `tests/run_programs_olc.sh`。

### 多文件示例

```bash
bash examples/olc -o examples/out/multi.elf \
    examples/programs/multi_file_main.ol \
    examples/programs/multi_file_lib.ol
./examples/out/multi.elf; echo $?
# 42
```

每个 `.ol` 编译为 `.oobj`，然后与运行时一并链接。详见 [docs/learn/tutorial_zh.md](../docs/learn/tutorial_zh.md)。

---

[返回文档](../docs/README_zh.md)
