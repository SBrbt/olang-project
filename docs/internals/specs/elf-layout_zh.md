# 示例：ELF 外形输出（平台包）

**[English](elf-layout.md)** | **中文**

---

### 概述

`alinker` **本身**不会「内置输出 ELF」：它只按链接脚本里的 `layout` 在指定偏移写字节。仓库里 **Linux x86_64 ELF 可执行文件** 示例（`examples/linux_x86_64/link/linux_elf_exe.json`）用 `write_blob` 写入与 **64 位 ELF** 头、程序头表一致的十六进制，再用 `write_payload` 写入合并后的映像。对照：`examples/bare_x86_64/link/raw.bin.json` 是 **x86 裸机** 风格的扁平映像——无容器头、无前置桩，负载从文件偏移 0 起写（见 `examples/bare_x86_64/README_zh.md`）。

### 布局（示例 `linux_elf_exe.json`）

```
┌─────────────────────┐ 0x0
│ layout 写入的字节   │  （脚本中的 ELF 头 / Phdr 等）
├─────────────────────┤
│ 0x1000 起的负载     │  call_stub_hex 桩 + 各段数据
│   .text / .rodata … │
└─────────────────────┘
```

### 段（示例）

典型 `linux_elf_exe.json` 使用两个可加载段（RX、RW）；具体 `file_off`、`vaddr`、标志位均以脚本为准。

### 入口符号

用于修补与入口相关的逻辑、符号名为脚本里的 **`"entry"`** 字符串，**不是**编译器写死。

```json
{
  "entry": "_start"
}
```

### 入口桩（`call_stub_hex`）

当 **`prepend_call_stub`** 为 true 时，脚本 **必须** 提供 **`call_stub_hex`**：放在合并段数据之前的原始字节。`alinker` **不再**内置默认桩；只做复制，且若首字节为 `0xE8`，会把 **+1** 处的 rel32 补成对 `"entry"` 符号的 `call`。

**示例** `linux_elf_exe.json` 中的 14 字节桩，在 x86-64 Linux 上对应「`call` 入口 → 再用 `main` 返回值做 syscall 退出」一类行为，但语义来自 **脚本里的十六进制**，不是链接器内写死。

### 运行时对象（`examples/linux_x86_64/olc`）

常见输入：

- `krt.kasm.oobj` — 定义 `_start`（`krt_init` → `main` → `krt_fini`）
- `olrt.kasm.oobj` — 编译器辅助；**OLang ABI** 见 `olrt.kasm`
- `libposix.kasm.oobj` — 系统调用封装（`syscall`），不是 libc

在使用示例桩 + `krt` 时的大致顺序：

1. 内核加载进程；ELF 入口（由脚本写入的头指定）指向负载起点（桩 + …）。
2. 按 **`call_stub_hex`** 的字节执行。
3. 再进入 `krt.kasm` 中的 `_start`。

---

[返回](../README_zh.md)
