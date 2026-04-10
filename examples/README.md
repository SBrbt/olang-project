# 示例（最小）

## 目录结构

| 路径 | 作用 |
|------|------|
| [`programs/`](programs/) | `ex_*.ol` 单文件测试；`multi_file_main.ol` / `multi_file_lib.ol` 双单元链接 |
| [`linux_x86_64/`](linux_x86_64/) | Linux x86_64 平台包：`link/*.json`、`asm/**` |
| [`olc`](olc) | bash 驱动（调用 `bin/olang`、`kasm`、`alinker`） |
| `out/` | 产物目录（勿提交） |

测试脚本已迁移到 `toolchain/tests/`：`run_programs_olc.sh`、`check_olang_bounds.sh`、`check_examples.sh`。

**期望退出码与 stdout** 以 [`../tests/run_programs_olc.sh`](../tests/run_programs_olc.sh) 为准（`make check` 会执行）；下表便于阅读，不一致时以脚本为准。

| 程序 (`ex_*.ol`) | 期望 |
|------------------|------|
| `ex_hello` | 退出码 `0`，stdout 含 `Hello from OLang` |
| `ex_write_ok` | 退出码 `0`，stdout 含 `OK` |
| `ex_stdout_two` | 退出码 `0`，stdout 含 `AB` |
| 其余 `programs/ex_*.ol`（含 `ex_rt_*`） | 退出码 `0`（`main` 内自检） |

## 鲁棒性用例（`ex_rt_*`）

- 前缀 **`ex_rt_`**：编译器/后端回归场景。
- 默认 **`return cast<i32>(0)`** 表示通过。
- Phase1 无对已有 `let` 的赋值；计数场景可能用递归代替可变循环。

新增 `ex_*.ol` 时脚本会**自动**编译运行；需要特殊 stdout/退出码断言时编辑 `tests/run_programs_olc.sh`。

## `olc` 与多文件

- **`examples/olc -o out.elf a.ol [b.ol …]`**：每单元 `.oobj` 后与 runtime 一并链接。详见 [specs/olang-packaging.md](../specs/olang-packaging.md)。
- **双文件**：`programs/multi_file_main.ol` + `multi_file_lib.ol`（`ex_*.ol` 之外的固定名字），由 `tests/run_programs_olc.sh` 末尾链接并期望退出码 `42`。

```bash
cd toolchain
make check
# 单独编译运行一个样例：
bash examples/olc -o /tmp/a.elf examples/programs/ex_rt_hex_literal.ol
```

产物在 **`examples/out/`**（勿提交）。
