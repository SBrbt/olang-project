# 预处理器（olprep）

**[English](preproc.md)** | **中文**

---

`bin/olprep` 是一个**体量很小、与具体语言无关**的、按行处理的预处理器。它**不属于** OLang 语法的一部分；编译器只看到 `olprep` 的输出。

### 在工具链中的位置

`examples/olc` 对每个源 `.ol` 依次执行：

1. `olprep --in <源文件> -o <工作目录>/ppN.ol -I <仓库>/examples/include`
2. `olang --in <工作目录>/ppN.ol -o <工作目录>/uN.oobj`

默认的 `-I` 使源码可写 `#include "posix_abi.ol"`，其中声明与 [`libposix.kasm`](../../examples/linux_x86_64/asm/lib/libposix.kasm) 中的符号一致（见 [`examples/include/posix_abi.ol`](../../examples/include/posix_abi.ol)）。
3. … 随后仍是 `kasm` / `alinker`。

### 已实现的指令

- `#include "path"` — 在此处插入被解析文件的**全文**。相对路径先相对**包含该 `#include` 的文件所在目录**解析，再按顺序尝试各 `-I` 目录。允许绝对路径。

其它以 `#` 开头的指令均不支持。

### 命令行

```
olprep --in <file> -o <out> [-I <dir>]...
```

### 安全性

- **循环包含**会被检测（同一规范路径出现在 include 栈上）并报错。
- **最大包含深度**为 64。

### 诊断与行号

**没有** `#line` 指令，也**没有**把诊断映射回展开前行号。编译器报错针对的是**预处理之后**的文件（例如工作目录下的 `pp0.ol`）。调试时请直接打开该文件对照。

### `olc` 工作目录

若使用 `-o /path/to/foo.elf`，中间产物写入：

`/path/to/foo.olc.d/`

`olc` **不会**删除该目录（会保留预处理后的 `.ol`、`.oobj` 以及组装的 runtime 目标等）。

---

[返回](README_zh.md)
