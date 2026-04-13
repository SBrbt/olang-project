# 示例

**[English](README.md)** | **中文**

---

### 目录结构

| 路径 | 作用 |
|------|------|
| [`linux_x86_64/`](linux_x86_64/) | **Linux x86_64** 平台包：[`olc`](linux_x86_64/olc)、[`programs/`](linux_x86_64/programs/)、[`include/`](linux_x86_64/include/)、[`link/linux_elf_exe.json`](linux_x86_64/link/linux_elf_exe.json)、[`asm/`](linux_x86_64/asm/) |
| [`bare_x86_64/`](bare_x86_64/) | **x86_64 裸机** 链接脚本示例（扁平原始映像；与 Linux 无关） |

使用 Linux 驱动时的产物放在 `linux_x86_64/out/`（勿提交；测试或本地运行会创建）。

### 快速开始（Linux ELF）

```bash
make all
bash examples/linux_x86_64/olc -o examples/linux_x86_64/out/hello.elf examples/linux_x86_64/programs/ex_hello.ol
./examples/linux_x86_64/out/hello.elf
```

### 期望结果

退出码与 stdout 以 [`tests/olang/run_programs_olc.sh`](../tests/olang/run_programs_olc.sh) 为准（`make check` 会执行）；学习文档里的表便于阅读，不一致时以脚本为准。

| 程序 (`ex_*.ol`) | 期望 |
|------------------|------|
| `ex_hello` | 退出码 `0`，stdout 含 `Hello from OLang` |
| `ex_write_ok` | 退出码 `0`，stdout 含 `OK` |
| `ex_stdout_two` | 退出码 `0`，stdout 含 `AB` |
| 其余 `ex_*.ol`（含 `ex_rt_*`） | 退出码 `0`（`main` 内自检） |

### 鲁棒性用例（`ex_rt_*`）

- 前缀 **`ex_rt_`**：编译器/后端回归场景。
- 默认 **`return i32(0)`** 表示通过。
- Phase 1 无对已有 `let` 的赋值；计数场景可能用递归代替可变循环。

在 `linux_x86_64/programs/` 下新增 `ex_*.ol` 会被测试脚本自动收录；需要特殊 stdout/退出码断言时编辑 `tests/olang/run_programs_olc.sh`。

### 多文件示例

```bash
bash examples/linux_x86_64/olc -o examples/linux_x86_64/out/multi.elf \
    examples/linux_x86_64/programs/multi_file_main.ol \
    examples/linux_x86_64/programs/multi_file_lib.ol
./examples/linux_x86_64/out/multi.elf; echo $?
# 42
```

详见 [docs/learn/tutorial_zh.md](../docs/learn/tutorial_zh.md)。

---

[返回文档](../docs/README_zh.md)
