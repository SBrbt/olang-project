# 测试

**[English](README.md)** | 中文

---

### 运行全部测试

```bash
make check
```

顺序大致为：链接脚本单元测试（`bin/link_script_test`）、alinker、kasm、预处理器，最后是 OLang 集成（用 `examples/olc` 跑完 `examples/programs/ex_*.ol`、`tests/olang/olang_*.ol`、多文件链接，以及预期失败用例）。

### 快速子集（仅 OLang + 示例）

`tests/check_examples.sh` 会 `make clean && make all`，然后只跑 `tests/olang/` 下的两个脚本。**不会**跑链接器、kasm、预处理器测试 —— 发版前请用 `make check`。

### 目录结构

```
tests/
├── README.md
├── check_examples.sh          # 快速路径：仅 OLang 集成（需先完整构建）
├── link_script_test.c         # 链接脚本解析；Makefile 生成 bin/link_script_test
├── olang/
│   ├── run_programs_olc.sh    # ex_*.ol + tests/olang/olang_*.ol + multi_file 链接
│   ├── check_olang_bounds.sh  # 预期编译失败（olang_fail/）
│   ├── olang_*.ol             # 额外程序（路径写在 run_programs_olc.sh 的数组里）
│   └── olang_fail/            # 必须编译失败的输入
├── alinker/
│   ├── smoke.sh
│   ├── pc64.sh
│   └── multi_obj.sh
├── kasm/
│   ├── label_comment.sh
│   └── bytes_tab.sh
├── preproc/
│   └── include.sh             # olprep #include
└── fixtures/                  # 各脚本共享的测试数据
```

### 按组件

| 区域 | 作用 |
|------|------|
| **OLang** | `run_programs_olc.sh` 编译并运行每个 `ex_*.ol`，再运行 `tests/olang/olang_*.ol`，最后多文件链接（见脚本里的多行 `OK:`）。`check_olang_bounds.sh` 检查 `olang_fail/*.ol` 必须编译失败。 |
| **alinker** | `smoke.sh`、`pc64.sh`、`multi_obj.sh` —— 使用 `fixtures/` 下的 `.oobj` 与 JSON。 |
| **kasm** | 汇编器边界情况；临时目录在 `fixtures/`。 |
| **preproc** | `include.sh` —— `#include "..."` 与 golden `expected.ol` 一致。 |
| **link script** | `link_script_test.c` 覆盖 alinker 使用的 C 解析逻辑。 |

### 新增测试

1. 新示例：添加 `examples/programs/ex_*.ol`（由通配符收录）。若在 `tests/olang/` 下增加额外程序，须在 `run_programs_olc.sh` 的 `TESTS_OLANG_SRC` 数组里加入路径（须以退出码 0 表示成功）。若退出码或 stdout 需要特殊断言，改 `run_programs_olc.sh`。
2. 组件级测试：在对应子目录加脚本，fixture 放在 `fixtures/<name>/`，在顶层 `Makefile` 的对应 `check-*` 规则里增加一行 `bash tests/...`。

---

[返回文档索引](../docs/README.md)
