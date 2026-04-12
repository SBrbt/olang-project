# x86_64 裸机（仅链接脚本）

**[English](README.md)** | **中文**

---

本目录**不**绑定 Linux：只放 **alinker** 用的 **扁平原始映像** 示例（无 ELF、无操作系统约定）。

| 路径 | 作用 |
|------|------|
| [`link/raw.bin.json`](link/raw.bin.json) | 示例：单段、`prepend_call_stub` 为 false、从文件偏移 0 写负载；可按引导程序或仿真器改 `vaddr` 与段列表。 |

另见：[docs/internals/specs/elf-layout_zh.md](../../docs/internals/specs/elf-layout_zh.md)。
